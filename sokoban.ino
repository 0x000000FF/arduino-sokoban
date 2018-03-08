// include the necessary libraries
#include <Esplora.h>
#include <SPI.h>
// #include <SD.h>
#include <TFT.h>            // Arduino LCD library

// the Esplora pin connected to the chip select line for SD card
#define SD_CS    8
#define MAPCELLS_X 8
#define MAPCELLS_Y 8

#define BOX  5  //the box weight
#define PER  7  //the person weight(power)
#define TGT  -2 //the target weight
#define WAL  10 //the wall weight

//person can push the object forward whitch is lighter than himself,for example,box(5) is 
//lighter than person(7),but two boxes(5+5) or a wall(10) is too heavy to move.the grand 
//weights 0,and the target position weights -2,so it can be detected when box on the target 
//position.there are two maps,one is the base map means topography,all the cells are 
//unmoveable,one is complete map,contains topography and moveable objects(person and boxes),
//when game started,the complete map was copyed to a temporary map(map_tem),and the moveable
//objects can move on this temporary map 

unsigned int person_pos[2] = {-1,-1};

static int theround = 0;

const int game_map_base[2][MAPCELLS_X][MAPCELLS_Y] = { 
                          { {999,999,999,WAL,WAL,WAL,WAL,999},
                            {999,WAL,WAL,WAL, 0 , 0 ,WAL,999},
                            {999,WAL, 0 , 0 , 0 , 0 ,WAL,999},
                            {WAL,WAL, 0 ,WAL, 0 ,WAL,WAL,WAL},
                            {WAL, 0 , 0 ,WAL, 0 ,WAL,TGT,WAL},
                            {WAL, 0 ,WAL, 0 , 0 , 0 , 0 ,WAL},
                            {WAL, 0 , 0 , 0 , 0 , 0 , 0 ,WAL},
                            {WAL,WAL,WAL,WAL,WAL,WAL,WAL,WAL}},

                          { {WAL,WAL,WAL,WAL,WAL,WAL,WAL,WAL},
                            {WAL, 0 , 0 , 0 , 0 , 0 , 0 ,WAL},
                            {WAL, 0 , 0 , 0 , 0 ,TGT, 0 ,WAL},
                            {WAL, 0 , 0 ,WAL,WAL,WAL,WAL,WAL},
                            {WAL,WAL, 0 , 0 ,WAL,999,999,999},
                            {WAL, 0 , 0 , 0 ,WAL,999,999,999},
                            {WAL, 0 , 0 ,TGT,WAL,999,999,999},
                            {WAL,WAL,WAL,WAL,WAL,999,999,999}}
                          };

const int game_map_obj[2][MAPCELLS_X][MAPCELLS_Y] = {  //10:wall 0:grand 5:box 7:person -2:target
                          { {999,999,999,WAL,WAL,WAL,WAL,999},
                            {999,WAL,WAL,WAL, 0 ,PER,WAL,999},
                            {999,WAL, 0 , 0 ,BOX, 0 ,WAL,999},
                            {WAL,WAL, 0 ,WAL, 0 ,WAL,WAL,WAL},
                            {WAL, 0 , 0 ,WAL, 0 ,WAL,TGT,WAL},
                            {WAL, 0 ,WAL, 0 , 0 , 0 , 0 ,WAL},
                            {WAL, 0 , 0 , 0 , 0 , 0 , 0 ,WAL},
                            {WAL,WAL,WAL,WAL,WAL,WAL,WAL,WAL}},

                          { {WAL,WAL,WAL,WAL,WAL,WAL,WAL,WAL},
                            {WAL, 0 , 0 , 0 , 0 , 0 ,PER,WAL},
                            {WAL, 0 , 0 , 0 ,BOX,TGT, 0 ,WAL},
                            {WAL, 0 , 0 ,WAL,WAL,WAL,WAL,WAL},
                            {WAL,WAL, 0 , 0 ,WAL,999,999,999},
                            {WAL, 0 ,BOX, 0 ,WAL,999,999,999},
                            {WAL, 0 , 0 ,TGT,WAL,999,999,999},
                            {WAL,WAL,WAL,WAL,WAL,999,999,999}}
                          };

int map_temp[MAPCELLS_X][MAPCELLS_Y] = {0};


// bool load_resource()
// {
//   Box = EsploraTFT.loadImage("box.bmp");
//   if (Box.isValid() == false)
//   {return false;}
//   Box2 = EsploraTFT.loadImage("box2.bmp");
//   if (Box2.isValid() == false)
//   {return false;}
//   Target = EsploraTFT.loadImage("target.bmp");
//   if (Box.isValid() == false)
//   {return false;}
//   Wall = EsploraTFT.loadImage("wall.bmp");
//   if (Box.isValid() == false)
//   {return false;}
//   Person = EsploraTFT.loadImage("person.bmp");
//   if (Box.isValid() == false)
//   {return false;}
//   Grand = EsploraTFT.loadImage("grand.bmp");
//   if (Box.isValid() == false)
//   {return false;}

//   return true;
// }
static const unsigned char PROGMEM bitmap_target[] = {
                        0x00,0x00,0x03,0xC0,0x0F,0xF0,0x1F,0xF8,
                        0x3C,0x3C,0x38,0x1E,0x70,0x0E,0x70,0x0E,
                        0x70,0x0E,0x70,0x0E,0x38,0x1E,0x3C,0x3C,
                        0x1F,0xF8,0x0F,0xF0,0x07,0xE0,0x00,0x00};

static const unsigned char PROGMEM bitmap_box[] = {
                        0x7F,0xFE,0xFF,0xFF,0xFF,0xFF,0xF0,0x0F,
                        0xE8,0x17,0xE4,0x27,0xE2,0x47,0xE1,0x87,
                        0xE1,0x87,0xE2,0x47,0xE4,0x27,0xE8,0x17,
                        0xF0,0x0F,0xFF,0xFF,0xFF,0xFF,0x7F,0xFE};

static const unsigned char PROGMEM bitmap_box2[] = {
                        0x7F,0xFE,0xFF,0xFF,0xFF,0xFF,0xF0,0x0F,
                        0xE8,0x17,0xE4,0x27,0xE2,0x47,0xE1,0x87,
                        0xE1,0x87,0xE2,0x47,0xE4,0x27,0xE8,0x17,
                        0xF0,0x0F,0xFF,0xFF,0xFF,0xFF,0x7F,0xFE};
static const unsigned char PROGMEM bitmap_wall[] = {
                        0xFF,0xFF,0x80,0x01,0x80,0x01,0x80,0x01,
                        0x80,0x01,0x80,0x01,0x80,0x01,0xFF,0xFF,
                        0xFF,0xFF,0x01,0x80,0x01,0x80,0x01,0x80,
                        0x01,0x80,0x01,0x80,0x01,0x80,0xFF,0xFF};

static const unsigned char PROGMEM bitmap_person[] = {
                        0x01,0x80,0x07,0xE0,0x0F,0xF0,0x0E,0xB0,
                        0x07,0xE0,0x07,0xE0,0x33,0xC2,0x1D,0x9E,
                        0x07,0xF0,0x03,0xC0,0x01,0x80,0x03,0xC0,
                        0x06,0x60,0x0C,0x20,0x38,0x18,0x60,0x0C};

static const unsigned char PROGMEM bitmap_grand[] = {
                        0x55,0x55,0xAA,0xAA,0x55,0x55,0xAA,0xAA,
                        0x55,0x55,0xAA,0xAA,0x55,0x55,0xAA,0xAA,
                        0x55,0x55,0xAA,0xAA,0x55,0x55,0xAA,0xAA,
                        0x55,0x55,0xAA,0xAA,0x55,0x55,0xAA,0xAA,};

void draw_cell(int type,int x,int y)
{
  if (type == 0)
  {
    EsploraTFT.stroke(127, 255, 127);
    EsploraTFT.fill(127, 255, 127);
    EsploraTFT.rect(x*16+17, y*16+1,16, 16);
    EsploraTFT.drawBitmap(x*16+17, y*16+1, bitmap_grand, 16, 16, 0x00A0);
  }
  else if (type == WAL)
  {
    EsploraTFT.stroke(34,34,178);
    EsploraTFT.fill(34,34,178);
    EsploraTFT.rect(x*16+17, y*16+1,16, 16);
    EsploraTFT.drawBitmap(x*16+17, y*16+1, bitmap_wall, 16, 16, 0x22B2);
  }
  else if (type == TGT)
  {
    EsploraTFT.stroke(127, 255, 127);
    EsploraTFT.fill(127, 255, 127);
    EsploraTFT.rect(x*16+17, y*16+1,16, 16);
    EsploraTFT.drawBitmap(x*16+17, y*16+1, bitmap_target, 16, 16, 0x0F00);
  }
  else if (type == BOX)
  {
    EsploraTFT.stroke(32,65,218);
    EsploraTFT.fill(32,65,218);
    EsploraTFT.rect(x*16+17, y*16+1,16, 16);
    EsploraTFT.drawBitmap(x*16+17, y*16+1, bitmap_box, 16, 16, 0xA5FF);
  }
  else if (type == PER)
  {
    EsploraTFT.stroke(127, 255, 127);
    EsploraTFT.fill(127, 255, 127);
    EsploraTFT.rect(x*16+17, y*16+1,16, 16);
    EsploraTFT.drawBitmap(x*16+17, y*16+1, bitmap_person, 16, 16, 0x0000);
    person_pos[0] = x;
    person_pos[1] = y;
  }
  else if (type == (BOX + TGT))
  {
    EsploraTFT.stroke(255, 255, 127);
    EsploraTFT.fill(255, 255, 127);
    EsploraTFT.rect(x*16+17, y*16+1,16, 16);
    EsploraTFT.drawBitmap(x*16+17, y*16+1, bitmap_box2, 16, 16, 0xFF00);
  }
}

void draw_map()
{
  for (int y = 0;y < 8;y++)
  {
    for (int x = 0;x < 8;x++)
    {
      draw_cell(game_map_obj[theround][y][x], x, y);
      map_temp[y][x] = game_map_obj[theround][y][x];
      // Serial.print (map_temp[y][x]);
    }
  }
}

bool move(unsigned char dir)
{
  int x,y;
  x = person_pos[0];
  y = person_pos[1];
  int xx = x;
  int yy = y;
  if (dir == 'u')
  {
    y--;
    yy >= 2 ? yy-=2 : yy--;
  }
  else if (dir == 'd')
  {
    y++;
    yy <= 5 ? yy+=2 : yy++;
  }
  else if (dir == 'l')
  {
    x--;
    xx >= 2 ? xx-=2 : xx--;
  }
  else if (dir == 'r')
  {
    x++;
    xx <= 5 ? xx+=2 : xx++;
  }

  // Serial.print("Next step :");
  // Serial.print(x);
  // Serial.print(",");
  // Serial.println(y);
  // Serial.println(map_temp[y][x]);
  // Serial.println(map_temp[yy][xx]);

  if (map_temp[y][x] <= 0) //moveable
  {
    draw_cell(game_map_base[theround][person_pos[1]][person_pos[0]], person_pos[0], person_pos[1]);
    map_temp[person_pos[1]][person_pos[0]] = game_map_base[theround][person_pos[1]][person_pos[0]];
    draw_cell(PER, x, y);
    map_temp[y][x] += PER;
    person_pos[0] = x;
    person_pos[1] = y;
    return true;
  }

  if (PER < (map_temp[y][x] + map_temp[yy][xx])) //there is a unmoveable cell,
  {
    return false;
  }

  else if (map_temp[y][x] - game_map_base[theround][y][x] == BOX) //there is a box
  {
    draw_cell(game_map_base[theround][person_pos[1]][person_pos[0]], person_pos[0], person_pos[1]);
    map_temp[person_pos[1]][person_pos[0]] = game_map_base[theround][person_pos[1]][person_pos[0]];
    draw_cell(PER, x, y);
    map_temp[y][x] = PER;
    person_pos[0] = x;
    person_pos[1] = y;
    map_temp[yy][xx] += BOX;
    draw_cell(map_temp[yy][xx], xx, yy);
    return true;
  }

  return false;
}


bool judge()
{
  int btn = HIGH;
  for (int i = 0;i < MAPCELLS_X;i++)
  {
    for (int j = 0;j < MAPCELLS_Y;j++)
    {
      if (game_map_base[theround][j][i] == TGT)
      {
        if (map_temp[j][i] != TGT + BOX)
        {
          return false;
        }
      }
    }
  }
  EsploraTFT.background(0, 0, 255);
  EsploraTFT.setTextSize(3);
  EsploraTFT.stroke(0, 255, 255);
  EsploraTFT.text("YOU WIN",20,50);

  while (btn == HIGH)
  {
    btn = Esplora.readButton(SWITCH_1);
    delay (100);
  }
  return true;
}

void setup() {
  EsploraTFT.begin();
  // Serial.begin(9600);

  EsploraTFT.background(255, 255, 255);
  // if (!load_resource()) 
  // {
    // Serial.println("load resources failed!");
    // return;
  // }
//  EsploraTFT.drawBitmap(0,0,bitmap,16,16,127);
}

void start_game()
{
  int btn = 1;
  int index = 0;
  int last_index = 0;
  const int menu_number = 2;
  char* menu[2] = {"1st round","2nd round"};
  static int yValue = Esplora.readJoystickY();
   
  EsploraTFT.background(127, 255, 127);
  EsploraTFT.setTextSize(3);
  EsploraTFT.stroke(255, 0, 255);
  EsploraTFT.text("SOKOBAN",20,20);
  EsploraTFT.setTextSize(2);

  EsploraTFT.setTextSize(1);
  EsploraTFT.stroke(0, 0, 0);
  EsploraTFT.text("By Qi.Wenxin,Luo.Jiaqing",10,110);
  EsploraTFT.setTextSize(2);

  for (int i = 0;i < menu_number;i++)
  {
    if (index == i)
    {
      EsploraTFT.stroke(0,0,255);
//      String s = ">" + menu[i];
      EsploraTFT.text(">", 6, 50+(i*16));
      EsploraTFT.text(menu[i], 20, 50+(i*16));
      EsploraTFT.stroke(255,0,0);
    }
    else
    {
      EsploraTFT.text(menu[i], 20, 50+(i*16));
    }
  }

  while (1)
  {
    yValue = Esplora.readJoystickY();
    btn = Esplora.readButton(SWITCH_1);

    if (btn == LOW)
    {
      theround = index;
      return ;
    }

    if (yValue > 400)
    {
      last_index = index;
      ++index >= menu_number ? index=0 : index;
    }
    else if (yValue < -400)
    {
      last_index = index;
      --index < 0 ? index=menu_number-1 : index;
    }

    else
    {
      continue;
    }

    EsploraTFT.stroke(127, 255, 127);
    EsploraTFT.fill(127, 255, 127);
    EsploraTFT.rect(6, 50+(last_index*16),16, 16);
    EsploraTFT.stroke(255,0,0);
    EsploraTFT.text(menu[last_index], 20, 50+(last_index*16));

    EsploraTFT.stroke(0,0,255);
    EsploraTFT.text(">", 6, 50+(index*16));
    EsploraTFT.text(menu[index], 20, 50+(index*16));

    delay (200);
  }
}

void run_game()
{
  EsploraTFT.background(127, 127, 127);
  draw_map();

  static int xValue = Esplora.readJoystickX();
  static int yValue = Esplora.readJoystickY();
  while (1)
  {
    xValue = Esplora.readJoystickX();
    yValue = Esplora.readJoystickY();
    if (xValue > 400)
    {
      move('l');
      // Serial.println("move left");
    }
    else if (xValue < -400)
    {
      move('r');
      // Serial.println("move right");
    }
    if (yValue > 400)
    {
      move('d');
      // Serial.println("move down");
    }
    else if (yValue < -400)
    {
      move('u');
      // Serial.println("move up");
    }

    if (judge())
    {
      return;
    }
    delay (200);
  }
}

void loop() {
  start_game();

  run_game();
}