// #include "mbed.h"
// #include "Adafruit_SSD1306.h"

//Switch input definition
#define SW_PIN_A p21
#define SW_PIN_B p22
#define SW_PIN_C p23
#define SW_PIN_D p24

//Sampling period for the switch oscillator (us)
#define SW_PERIOD 20000

//Display interface pin definitions
#define D_MOSI_PIN p5
#define D_CLK_PIN p7
#define D_DC_PIN p8
#define D_RST_PIN p9
#define D_CS_PIN p10


//an SPI sub-class that sets up format and clock speed
class SPIPreInit : public SPI {
  public:
    SPIPreInit(PinName mosi, PinName miso, PinName clk) : SPI(mosi,miso,clk){
      format(8,3);
      frequency(2000000);
    };
};

//Interrupt Service Routine prototypes (functions defined below)
void sedgea();
void sedgeb();
void sedgec();
void sedged();
void tout();


//Output for the alive LED
DigitalOut alive(LED1);

//External interrupt input from the switch oscillator
InterruptIn swina(SW_PIN_A);
InterruptIn swinb(SW_PIN_B);
InterruptIn swinc(SW_PIN_C);
InterruptIn swind(SW_PIN_D);

//Switch sampling timer
Ticker swtimer;

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
volatile bool astate = false;
volatile bool bstate = false;
volatile bool cstate = false;
volatile bool dstate = false;

bool reset = false;

//Initialise SPI instance for communication with the display
SPIPreInit gSpi(D_MOSI_PIN,NC,D_CLK_PIN); //MOSI,MISO,CLK

//Initialise display driver instance
Adafruit_SSD1306_Spi gOled1(gSpi,D_DC_PIN,D_RST_PIN,D_CS_PIN,64,128); //SPI,DC,RST,CS,Height,Width

using namespace std;

void print_grid(vector<char> grid, int col_input_piece);
void insert_choose(vector<char>& grid, char choose, int column);
bool win(vector<char> grid);
bool complete_grid(vector<char> grid);
bool valid_insert(vector<char> grid, int column);
void movecursor(int& col_input_piece, bool right);

int main(){
  //Initialisation
  gOled1.setRotation(2); //Set display rotation
  gOled1.clearDisplay();

  //Attach switch oscillator counter ISR to the switch input instance for a rising edge
  swina.rise(&sedgea);
  swinb.rise(&sedgeb);
  swinc.rise(&sedgec);
  swind.rise(&sedged);

  //Attach switch sampling timer ISR to the timer instance with the required period
  swtimer.attach_us(&tout, SW_PERIOD);

  while (1) {
    reset = false;

    int num_of_rows = 6;
    int num_of_col = 7;
    int col_input_piece = 0;
    char winner;
    int aholdcounter;
    int bholdcounter;
    int choldcounter;

    //initialise grid
    vector<char> grid;
    for (int i = 0; i < num_of_rows; i++)
        for (int j = 0; j < num_of_col; j++)
            grid.push_back('_');

    bool gameover = false;
    int counter = 0;
    bool comp_grid = false;

    print_grid(grid, col_input_piece);

    while(!gameover && !comp_grid){
      if (scounta<1720) {
        if (aholdcounter%5 == 0) {
          astate = true;
        }
        else {
          astate = false;
        }
        aholdcounter++;
      }
      else if (scounta>1740) {
        astate = false;
        aholdcounter = 0;
      }
      if (scountb<1775) {
        if (bholdcounter%5 == 0) {
          bstate = true;
        }
        else {
          bstate = false;
        }
        bholdcounter++;
      }
      else if (scountb>1790) {
        bstate = false;
        bholdcounter = 0;
      }
      if (scountc<1750) {
        if (choldcounter%20 == 0) {
          cstate = true;
        }
        else {
          cstate = false;
        }
        choldcounter++;
      }
      else if (scountc>1770) {
        cstate = false;
        choldcounter = 0;
      }
      if (scountd<3600) {
        dstate = true;
        reset = true;
        break;
      }
      else if (scountd>3650) {
        dstate = false;
      }

      print_grid(grid, col_input_piece);
      bool valid_input = false;

      if (astate || bstate) {
        movecursor(col_input_piece, astate);

      }

      if (cstate) {
        valid_input = valid_insert(grid, col_input_piece);
      }
      if (valid_input){
        if(counter % 2){
          insert_choose(grid, 'x' , col_input_piece);
        }
        else{
          insert_choose(grid, 'o' , col_input_piece);
        }
        gameover = win(grid);
        if (gameover) {
          if (counter%2)
            winner = 'x';
          else
            winner = 'o';
        }
        comp_grid = complete_grid(grid);
        col_input_piece = 0;
        counter++;
      }

      if (!gameover && !comp_grid){
        print_grid(grid, col_input_piece);
      }
    }

    print_grid(grid, col_input_piece);
    gOled1.clearDisplay();
    while (!reset) {
      if (gameover) {
        gOled1.setTextCursor(4,4);
        gOled1.printf("Winner Is: %c", winner);
        gOled1.display();
      }

      else {
        gOled1.setTextCursor(4,7);
        gOled1.printf("Draw");
        gOled1.display();
      }
      if (scountd<3600) {
        reset = true;
      }
    }
  }
}

void print_grid(vector<char> grid, int col_input_piece){
  int num_of_row = 6;
  int num_of_col = 7;
  //Set text cursor
  gOled1.setTextCursor(0,0);
  for (int i = 0; i < num_of_row; i++){
    for (int j = 0; j < num_of_col; j++)
      gOled1.printf("%c ",grid[num_of_col * i + j]);
    gOled1.printf("\n");
  }
  for (int i = 0; i < 7; i++){
    if (i == col_input_piece) {
      gOled1.printf("^");
      i = 7;
    }
    else {
      gOled1.printf("  ");
    }
  }
  gOled1.display();
}

void movecursor(int& col_input_piece, bool right){
  if (right) {
    col_input_piece++;
    col_input_piece = col_input_piece%7;
    gOled1.clearDisplay();
  }
  else {
    col_input_piece--;
    if (col_input_piece == -1) {
      col_input_piece = 6;
    }
    gOled1.clearDisplay();
  }
}

void insert_choose(vector<char>& grid, char choose, int column){
  int num_of_col = 7;
  for (int i = column; i < grid.size(); i = i + num_of_col){
    if (i + num_of_col >= grid.size()) {
      grid[i] = choose;
      gOled1.clearDisplay();
    }

    else
      if (grid[i + num_of_col] != '_'){
        grid[i] = choose;
        gOled1.clearDisplay();
        break;
      }
  }
}

bool win(vector<char> grid){
  int num_of_row = 6;
  int num_of_col = 7;

  for(int i = 0; i < num_of_row; i++){
    for (int j = 0; j < num_of_col; j++){
    int index = i * num_of_col + j;

    //checking on the col
    if (index < 20)
      if (grid[index] == grid[index + num_of_col] && grid[index] != '_' && grid[index] == grid[index + 2 * num_of_col] && grid[index] == grid[index + 3 * num_of_col])
        return true;

    //checking the row
    if (j < 4)
      if (grid[index] == grid[index + 1]  && grid[index] != '_' && grid[index] == grid[index + 2] && grid[index] == grid[index + 3])
        return true;


    //check the left to right diagonal
    if ( j < 4 && index < 21)
      if (grid[index] == grid[index + num_of_col + 1]  && grid[index] != '_' && grid[index] == grid[index + 2 * num_of_col + 2] && grid[index] == grid[index + 3 * num_of_col + 3])
        return true;

    //check the right to left diagonal
    if ( j > 2 && index < 21)
      if (grid[index] == grid[index + num_of_col - 1]  && grid[index] != '_' && grid[index] == grid[index + 2 * num_of_col - 2] && grid[index] == grid[index + 3 * num_of_col - 3])
        return true;
    }
  }
  return false;
}

bool complete_grid(vector<char> grid){
  for (int i = 0; i < grid.size(); i++)
    if (grid[i] == '_')
      return false;
    return true;
}

bool valid_insert(vector<char> grid, int column){
  if (grid[column] == '_')
    return true;
  return false;
}

//Interrupt Service Routine for rising edge on the switch oscillator input
void sedgea() {
  //Increment the edge counter
  scountera++;
}

void sedgeb() {
  //Increment the edge counter
  scounterb++;
}

void sedgec() {
  //Increment the edge counter
  scounterc++;
}

void sedged() {
  //Increment the edge counter
  scounterd++;
}

//Interrupt Service Routine for the switch sampling timer
void tout() {
  //Read the edge counter into the output register
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
