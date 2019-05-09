#include "mbed.h"
#include "Adafruit_SSD1306.h"

//Switch input definition
#define SW_PIN_A p21
#define SW_PIN_B p22
#define SW_PIN_C p23
#define SW_PIN_D p24

//Wave output pin definition
#define D_OUT_PIN p18
#define A_OUT_PIN p27

//Sampling period for the switch oscillator (us)
#define SW_PERIOD 20000

//Display interface pin definitions
#define D_MOSI_PIN p5
#define D_CLK_PIN p7
#define D_DC_PIN p8
#define D_RST_PIN p9
#define D_CS_PIN p10

#define PI 3.14159265358979323846

//Setup output pins
DigitalOut wave_out(D_OUT_PIN);
//AnalogOut aout(A_OUT_PIN);

//Counters for triangle and sine waves
volatile uint8_t wave_counter = 0;
float tri_wave[64];
float sin_wave[64];

//an SPI sub-class that sets up format and clock speed
class SPIPreInit : public SPI
{
public:
    SPIPreInit(PinName mosi, PinName miso, PinName clk) : SPI(mosi,miso,clk)
    {
        format(8,3);
        frequency(2000000);
    };
};

//Interrupt Service Routine prototypes (functions defined below)
void sedge_a();
void sedge_b();
void sedge_c();
void sedge_d();
void tout();

//Wave output interrupts
void squ_out();
void sin_out();
void tri_out();

//Misc functions
void set_ticker();


//Output for the alive LED
DigitalOut alive(LED1);

//External interrupt input from the switch oscillator
InterruptIn swin_a(SW_PIN_A);
InterruptIn swin_b(SW_PIN_B);
InterruptIn swin_c(SW_PIN_C);
InterruptIn swin_d(SW_PIN_D);

//Switch sampling timer
Ticker swtimer;

//Wave output tickers
Ticker squ_ticker;
Ticker tri_ticker;
Ticker sin_ticker;



//Registers for the switch counter, switch counter latch register and update flag
volatile uint16_t scountera=0;
volatile uint16_t scounta=0;
volatile uint16_t scounterb=0;
volatile uint16_t scountb=0;
volatile uint16_t scounterc=0;
volatile uint16_t scountc=0;
volatile uint16_t scounterd=0;
volatile uint16_t scountd=0;
volatile uint16_t update=0;

//Booleans to track current switch on/off state
volatile bool astate = false;
volatile bool bstate = false;
volatile bool cstate = false;
volatile bool dstate = false;

//Booleans to track switch press edge
volatile bool achange = false;
volatile bool bchange = false;
volatile bool cchange = false;
volatile bool dchange = false;
volatile bool setfreq = false;

//Counter to allow adjustments at different speeds
volatile uint16_t changecounter=0;

//Wave type variable, 0 - square wave, 1 - sine wave, 2 - triangle wave
volatile int wavetype = 0;

//Variables for selector frequency and current output frequency
volatile int selfreq = 0;
volatile int outfreq = 0;

//Initialise SPI instance for communication with the display
SPIPreInit gSpi(D_MOSI_PIN,NC,D_CLK_PIN); //MOSI,MISO,CLK

//Initialise display driver instance
Adafruit_SSD1306_Spi gOled1(gSpi,D_DC_PIN,D_RST_PIN,D_CS_PIN,64,128); //SPI,DC,RST,CS,Height,Width

int main() {
    //Initialisation
    gOled1.setRotation(2); //Set display rotation
    gOled1.clearDisplay();

    //Attach switch oscillator counter ISR to the switch input instance for a rising edge
    swin_a.rise(&sedge_a);
    swin_b.rise(&sedge_b);
    swin_c.rise(&sedge_c);
    swin_d.rise(&sedge_d);

    //Attach switch sampling timer ISR to the timer instance with the required period
    swtimer.attach_us(&tout, SW_PERIOD);

    //Fill sine and triangle arrays
    for (int i = 0; i < 64; i++) {
        sin_wave[i] = (0.5 + 0.5*sin(2.0*PI*(float(i)/float(64))));
        if (i<64/2) {
            tri_wave[i] = float(2.0*i/64);
        }
        else {
            tri_wave[i] = 2.0-float(2.0*i/64);
        }
    }

    //Default freq
    selfreq = outfreq = 8000;

    //Default square wave
    squ_ticker.attach_us(&squ_out, 1000000/outfreq/2);


    //Main loop
    while(1)
    {
        //Has the update flag been set?
        if (update) {
            //Clear the update flag
            update = 0;
            if (scounta < 1720) {
                if (!astate) {
//                    achange = true;
                    changecounter = 0;
                }
                if (!setfreq) {
                    changecounter++;
                    if (changecounter < 50) {
                        if (outfreq < 10000) {
                            outfreq++;
                        }
                        set_ticker();
                    }
                    else {
                        if (outfreq < 9980) {
                            outfreq += 20;
                        }
                        else {
                            outfreq = 10000;
                        }
                        set_ticker();
                    }
                }
                if (setfreq) {
                    changecounter++;
                    if (changecounter < 50) {
                        if (selfreq < 10000) {
                            selfreq++;
                        }
                    }
                    else {
                        if (selfreq < 9980) {
                            selfreq += 20;
                        }
                        else {
                            selfreq = 10000;
                        }
                    }
                }
                astate = true;
            }
            else if (scounta>1740) {
                if (astate) {
//                    achange = true;
                    changecounter = 0;
                }
                astate = false;
            }
            if (scountb<1760) {
                if (!bstate) {
//                    bchange = true;
                    changecounter = 0;
                }
                if (!setfreq) {
                    changecounter++;
                    if (changecounter < 50) {
                        if (outfreq > 1) {
                            outfreq--;
                        }
                        set_ticker();
                    }
                    else {
                        if (outfreq > 20) {
                            outfreq -= 20;
                        }
                        else {
                            outfreq = 1;
                        }
                        set_ticker();
                    }
                }
                if (setfreq) {
                    changecounter++;
                    if (changecounter < 50) {
                        if (selfreq > 1) {
                            selfreq++;
                        }
                    }
                    else {
                        if (selfreq > 20) {
                            selfreq -= 20;
                        }
                        else {
                            selfreq = 1;
                        }
                    }
                }
                bstate = true;
            }
            else if (scountb>1790) {
                if (bstate) {
//                    bchange = true;
                }
                bstate = false;
            }
            if (scountc<1750) {
                if (!cstate) {
                    cchange = true;
                }
                cstate = true;
            }
            else if (scountc>1770) {
                if (cstate) {
                    cchange = true;
                }
                cstate = false;
            }
            if (scountd<3250) {
                if (!dstate) {
                    dchange = true;
                }
                dstate = true;
            }
            else if (scountd>3290) {
                if (dstate) {
                    dchange = true;
                }
                dstate = false;
            }


            //if (cchange && cstate) {
//                wavetype++;
//                if (wavetype>2) {
//                    wavetype = 0;
//                }
//                switch (wavetype) {
//                    case 0:
//                        // Square Wave
//                        sin_ticker.detach();
//                        tri_ticker.detach();
//                        squ_ticker.attach_us(&squ_out, 1000000/outfreq/2);
//                        break;
//                    case 1:
//                        // Sine Wave
//                        squ_ticker.detach();
//                        tri_ticker.detach();
//                        sin_ticker.attach_us(&sin_out, 1000000/outfreq/64);
//                        break;
//                    case 2:
//                        // Tri Wave
//                        squ_ticker.detach();
//                        sin_ticker.detach();
//                        tri_ticker.attach_us(&tri_out, 1000000/outfreq/64);
//                        break;
//                }
//                cchange = false;
//            }
//
            if (dchange && dstate) {
                if (!setfreq) {
                    setfreq = true;
                    selfreq = outfreq;
                }
                else {
                    outfreq = selfreq;
                    setfreq = false;
                    set_ticker();
                    gOled1.clearDisplay();
                }
                dchange = false;
            }


            //Set text cursor
            gOled1.setTextCursor(0,0);

            //Write the latest switch osciallor counts
            gOled1.printf("\n%05u  ",scounta);
            if (astate) {
                gOled1.printf(" P");
            }
            else {
                gOled1.printf(" U");
            }
            gOled1.printf("\n%05u  ",scountb);
            if (bstate) {
                gOled1.printf(" P");
            }
            else {
                gOled1.printf(" U");
            }
            gOled1.printf("\n%05u  ",scountc);
            if (cstate) {
                gOled1.printf(" P");
            }
            else {
                gOled1.printf(" U");
            }
            gOled1.printf("\n%05u  ",scountd);
            if (dstate) {
                gOled1.printf(" P");
            }
            else {
                gOled1.printf(" U");
            }

            //Show wave output frequency
            gOled1.printf("\n\nOut Freq: %05u", outfreq);
            if (setfreq) {
                gOled1.printf("\nSel Freq: %05u", selfreq);
            }

            //Copy the display buffer to the display
            gOled1.display();

            //Toggle the alive LED
            alive = !alive;

        }
    }
}


//Interrupt Service Routine for rising edge on the switch oscillator input
void sedge_a() {
    //Increment the edge counter
    scountera++;
}

void sedge_b() {
    //Increment the edge counter
    scounterb++;
}

void sedge_c() {
    //Increment the edge counter
    scounterc++;
}

void sedge_d() {
    //Increment the edge counter
    scounterd++;
}

//Interrupt Service Routine for the switch sampling timer
void tout() {
    //Read the edge counters into the output register
    scounta = scountera;
    scountb = scounterb;
    scountc = scounterc;
    scountd = scounterd;
    //Reset the edge counter
    scountera = 0;
    scounterb = 0;
    scounterc = 0;
    scounterd = 0;
    //Trigger a display update in the main loop
    update = 1;
}

void squ_out() {
    wave_out = !wave_out;
}

void sin_out() {
//    aout = sin_wave[wave_counter];
    wave_counter++;
}

void tri_out() {
//    aout = tri_wave[wave_counter];
    wave_counter++;
}


//Updates ticker
void set_ticker() {
    switch(wavetype) {
        case 0: squ_ticker.attach_us(&squ_out, 1000000/outfreq/2); break;
        case 1: sin_ticker.attach_us(&sin_out, 1000000/outfreq/64); break;
        case 2: tri_ticker.attach_us(&tri_out, 1000000/outfreq/64); break;
    }
}
