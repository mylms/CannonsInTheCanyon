/*
 Name:		Cannons in the Canyon.ino
 Created:	31.05.2019 17:29:35
 Author:	Petr
*/

//RGB 565 color make here: http://www.barth-dev.de/online/rgb565-color-picker/



//some libraries
#include <Adafruit_ILI9341_STM.h>
#include <SPI.h>

byte appVersion = 1;
byte appRevision = 0;

//DISPLAY ILI9341
Adafruit_ILI9341_STM lcd = Adafruit_ILI9341_STM(PA0, PA2, PA1);  // (CS, DC, RST)
int baclightIntensity = 65535;	//actual backlight intensity

//IO
#define BACKLIGHT_PIN PA8	//LCD Backlight

#define BUTTON1LEFT_PIN PA11	//Button Left
#define BUTTON2FIRE_PIN PA15	//Button Ok (PB6)
#define BUTTON3RIGHT_PIN PA10	//Button Right (PB7)
#define BUTTON4UP_PIN PA9	//Button Up
#define BUTTON5DOWN_PIN PA12	//Button Down
#define BUTTON6A_PIN PB3	//Button A
#define BUTTON7B_PIN PB8	//Button B

//BUTTONS
bool btnLeft, btnOk, btnRight, btnUp, btnDown, btnA, btnB; //buttons (current state) 
bool lastBtnLeft, lastBtnOk, lastBtnRight, lastBtnUp, lastBtnDown, lastBtnA, lastBtnB; //last input's state
bool edgeBtnLeft, edgeBtnOk, edgeBtnRight, edgeBtnUp, edgeBtnDown, edgeBtnA, edgeBtnB; //edge state - it's true only one loop

bool buttonFastCounter;
int buttonLongHold; //how long is button pressed

unsigned long lastTime1; //timming
unsigned long presentTime; //current time

//GAME
#define TANK0_COLOR ILI9341_BLUE
#define TANK1_COLOR ILI9341_RED
#define TANK2_COLOR ILI9341_GREEN
#define TANK3_COLOR ILI9341_YELLOW
#define BACK_TERRAIN_COLOR 0x2945

#define MISSILE0_PRICE 5
#define MISSILE1_PRICE 10
#define MISSILE2_PRICE 25
#define MISSILE3_PRICE 50
#define MISSILE4_PRICE 15
#define MISSILE5_PRICE 40

byte terrain1[320];	//height of terrain (main)
byte terrain2[320];	//height of terrain (background)

byte tanksCount = 2;	//count of all tanks (min:2 | max:4)
int tankX[4];	//possition X
int tankY[4];	//possition Y
byte tankAngle[4];	//actual angle
int tankPower[4];	//actual power
int tankPoints[4];	//count
bool tankAlive[4];	//is this tank alive
int missileType = 0;	//type of missile
byte aliveTanks = 0;	//how namy tanks is alive
byte activeTank = 0;	//current active tank

byte winner = 0;
int winTankColor = TANK0_COLOR;

int addPoints = 0;	
int wind = 0;	//wind power
bool hit = false;	//tank was hit

int animationAngle = 0;

byte systemState = 0;	//actual system state 0 - menu, 1 - game, 3 - game menu


void setup(void) {
	//SET IO
	//Backlight
	pinMode(BACKLIGHT_PIN, PWM);
	pwmWrite(BACKLIGHT_PIN, baclightIntensity);

	//INPUTS
	pinMode(BUTTON1LEFT_PIN, INPUT_PULLUP);
	pinMode(BUTTON2FIRE_PIN, INPUT_PULLUP);
	pinMode(BUTTON3RIGHT_PIN, INPUT_PULLUP);
	pinMode(BUTTON4UP_PIN, INPUT_PULLUP);
	pinMode(BUTTON5DOWN_PIN, INPUT_PULLUP);
	pinMode(BUTTON6A_PIN, INPUT_PULLUP);
	pinMode(BUTTON7B_PIN, INPUT_PULLUP);

	//SET LCD
	lcd.begin();	//komunikace SPI
	lcd.setRotation(1);	//widescreen

	randomSeed(analogRead(PA3) + millis());

	//Make splashscreen
	GenerateSplashScreen();
	while (digitalRead(BUTTON2FIRE_PIN)) {
		//wait to press OK
		delay(1);
	}

	delay(100);

	lcd.setTextSize(1);	//Back standard font
}

// the loop function runs over and over again until power down or reset
void loop() {
	presentTime = millis(); //store time for timming

	//get inputs
	btnLeft = !digitalRead(BUTTON1LEFT_PIN);
	btnOk = !digitalRead(BUTTON2FIRE_PIN);
	btnRight = !digitalRead(BUTTON3RIGHT_PIN);
	btnUp = !digitalRead(BUTTON4UP_PIN);
	btnDown = !digitalRead(BUTTON5DOWN_PIN);
	btnA = !digitalRead(BUTTON6A_PIN);
	btnB = !digitalRead(BUTTON7B_PIN);

	//edge detection
	edgeBtnLeft = (btnLeft ^ lastBtnLeft) & btnLeft;
	edgeBtnOk = (btnOk ^ lastBtnOk) & btnOk;
	edgeBtnRight = (btnRight ^ lastBtnRight) & btnRight;
	edgeBtnUp = (btnUp ^ lastBtnUp) & btnUp;
	edgeBtnDown = (btnDown ^ lastBtnDown) & btnDown;
	edgeBtnA = (btnA ^ lastBtnA) & btnA;
	edgeBtnB = (btnB ^ lastBtnB) & btnB;

	//TIMER FOR FAST COUNTING
	if (presentTime - lastTime1 >= 50) {
		//timer for fast value counting
		lastTime1 = presentTime; //store last time

		//detected button
		if (btnLeft || btnRight || btnUp || btnDown) {
			buttonLongHold++;
		}
		else {
			buttonLongHold = 0;
		}

		if (buttonLongHold > 20) {
			//button is hold too long
			//this variable set time before fast counting
			buttonFastCounter = !buttonFastCounter;
		}
		else {
			buttonFastCounter = false;
		}
	}

	switch (systemState) {
	case 0:
		//PREPARE MAIN MENU
		DrawMenu();
		systemState++;
		break;
	case 1:
		//MAIN MENU
		if (edgeBtnB) {
			//set default values
			for (int i = 0; i < tanksCount; i++) {
				tankAngle[i] = 90;
				tankPower[i] = 50;
				tankAlive[i] = true;
				tankPoints[i] = 100;
			}

			MakeNewMap();
			systemState = 10;	//go to game
		}

		if (edgeBtnRight) {
			if (tanksCount < 4) {
				tanksCount++;
			}

			//draw square
			lcd.drawRect(5, 40, 80, 24, ILI9341_BLACK);
			lcd.drawRect(5, 40, 120, 24, ILI9341_BLACK);
			lcd.drawRect(5, 40, 160, 24, ILI9341_BLACK);
			lcd.drawRect(5, 40, tanksCount * 40, 24, ILI9341_WHITE);

		}

		if (edgeBtnLeft) {
			if (tanksCount > 2) {
				tanksCount--;
			}

			//draw square
			lcd.drawRect(5, 40, 80, 24, ILI9341_BLACK);
			lcd.drawRect(5, 40, 120, 24, ILI9341_BLACK);
			lcd.drawRect(5, 40, 160, 24, ILI9341_BLACK);
			lcd.drawRect(5, 40, tanksCount * 40, 24, ILI9341_WHITE);
		}

		break;
	case 10:
		//GAME

		//BTN ANGLE LEFT
		//CHECK EDGE AND FAST COUNTING
		if (edgeBtnLeft || (btnLeft && buttonFastCounter)) {
			buttonFastCounter = false;	//deactive fast count

			tankAngle[activeTank] -= 1;
			if (tankAngle[activeTank] < 1) {
				tankAngle[activeTank] = 1;
			}

			RedrawBehindTanks();
		}

		//BTN ANGLE RIGHT
		//CHECK EDGE AND FAST COUNTING
		if (edgeBtnRight || (btnRight && buttonFastCounter)) {
			buttonFastCounter = false;	//deactive fast count

			tankAngle[activeTank] += 1;
			if (tankAngle[activeTank] > 179) {
				tankAngle[activeTank] = 179;
			}

			RedrawBehindTanks();
		}

		//BTN POWER UP
		//CHECK EDGE AND FAST COUNTING
		if (edgeBtnUp || (btnUp && buttonFastCounter)) {
			buttonFastCounter = false;	//deactive fast count

			tankPower[activeTank] += 1;
			if (tankPower[activeTank] > 100) {
				tankPower[activeTank] = 100;
			}

			RedrawHid2();
		}

		//BTN POWER DOWN
		//CHECK EDGE AND FAST COUNTING
		if (edgeBtnDown || (btnDown && buttonFastCounter)) {
			buttonFastCounter = false;	//deactive fast count

			tankPower[activeTank] -= 1;
			if (tankPower[activeTank] < 10) {
				tankPower[activeTank] = 10;
			}

			RedrawHid2();
		}

		//GAME MENU
		if (edgeBtnB) {
			systemState = 20;	//go to game menu
			edgeBtnB = false;
		}

		//BTN FIRE
		//CHECK EDGE
		if (edgeBtnOk) {
			bool collision = false;
			float bulletX = tankX[activeTank] + 6.0;
			float bulletY = tankY[activeTank] - 5.0;
			float lastBulletX = bulletX;
			float lastBulletY = bulletY;

			float bulletPowerLeft = 0.0;
			float bulletPowerRight = 0.0;
			float bulletPowerUp = 0.0;
			float bulletPowerDown = 0.0;

			float xVelocity = tankPower[activeTank] * cos((float)tankAngle[activeTank] * DEG_TO_RAD) * -0.15;
			float yVelocity = tankPower[activeTank] * sin((float)tankAngle[activeTank] * DEG_TO_RAD) * -0.15;

			//price of one shot (depend on missile type)
			switch (missileType) {
			case 0:
				tankPoints[activeTank] -= MISSILE0_PRICE;
				bulletPowerLeft = 10;
				bulletPowerRight = 20;
				bulletPowerUp = 10;
				bulletPowerDown = 10;
				break;
			case 1:
				tankPoints[activeTank] -= MISSILE1_PRICE;
				bulletPowerLeft = 15;
				bulletPowerRight = 25;
				bulletPowerUp = 15;
				bulletPowerDown = 15;
				break;
			case 2:
				tankPoints[activeTank] -= MISSILE2_PRICE;
				bulletPowerLeft = 25;
				bulletPowerRight = 35;
				bulletPowerUp = 20;
				bulletPowerDown = 25;
				break;
			case 3:
				tankPoints[activeTank] -= MISSILE3_PRICE;
				bulletPowerLeft = 50;
				bulletPowerRight = 60;
				bulletPowerUp = 10;
				bulletPowerDown = 25;
				break;
			case 4:
				tankPoints[activeTank] -= MISSILE4_PRICE;
				bulletPowerLeft = 10;
				bulletPowerRight = 20;
				bulletPowerUp = 10;
				bulletPowerDown = 10;
				break;
			case 5:
				tankPoints[activeTank] -= MISSILE5_PRICE;
				bulletPowerLeft = 10;
				bulletPowerRight = 20;
				bulletPowerUp = 10;
				bulletPowerDown = 10;
				break;
			}


			while (!collision) {
				//hide trajectory
				//draw background color to last pixel
				if (lastBulletY < (terrain2[(int)lastBulletX] - 50)) {
					//above background terrain
					lcd.drawPixel(lastBulletX, lastBulletY, ILI9341_BLACK);
				}
				else {
					//on background terrain
					lcd.drawPixel(lastBulletX, lastBulletY, BACK_TERRAIN_COLOR);
				}


				//go until detect collision
				if (missileType == 4) {
					//special trajextory
					yVelocity += (abs(yVelocity) * 0.15) + 0.05; // apply gravity
					bulletX += (xVelocity * 0.75) + (wind * 0.01);  // update x position...apply wind
				}
				else {
					//standard trajectory
					yVelocity += (abs(yVelocity) * 0.1) + 0.05; // apply gravity
					bulletX += xVelocity + (wind * 0.04);  // update x position...apply wind
				}
				
				bulletY += yVelocity;  // update y position

				lcd.drawPixel(bulletX, bulletY, ILI9341_WHITE);	//draw point
				lastBulletX = bulletX;	//store last possition for redraw
				lastBulletY = bulletY;	//store last possition for redraw

				if (bulletX < 0 || bulletX > 320) {
					//out of screen
					collision = true;
				}

				if (bulletY <= 0 || bulletY > terrain1[(int)bulletX]) {
					//hit terrain
					for (int i = 0; i < tanksCount; i++) {
						//check all tanks
						if ((bulletX > tankX[i] - bulletPowerLeft) && (tankX[i] + bulletPowerRight > bulletX) && (bulletY > tankY[i] - bulletPowerDown) && (tankY[i] + bulletPowerUp > bulletY) && tankAlive[i]) {
							hit = true;	//some tank was hit
							aliveTanks--;	//some tank was deleted

							tankPoints[i] -= 15;	//killed tank lost 10 points
							tankAlive[i] = false;	//kill the tank

							if (i == activeTank) {
								addPoints = -15;	//selfshoot
							}
							else {
								addPoints = 15;
							}
						}
					}

					collision = true;
				}

				delay(45);
			}

			if (hit) {
				//tank was hit
				lcd.invertDisplay(true);	//make efect :)
				tankPoints[activeTank] += addPoints;	//add some points
				DamageTerrain(bulletX, 10);	//damage terrain - 10 = tank explosion
				hit = false;
			}

			DamageTerrain(bulletX, missileType);	//damage terrain by real missile
			RedrawBehindTanks();
			WindRecount();	//new wind intensity
			lcd.invertDisplay(false);	//turn off hit effect

			//check if game can continue
			byte avaliableTanks = 0;
			for (int i = 0; i < tanksCount; i++) {
				//how many tanks have more than 0 points
				if (tankPoints[i] > 0) {
					avaliableTanks++;
				}

				//kill alive tank without any points
				if ((tankPoints[i] < 1) && tankAlive[i]) {
					tankAlive[i] = false;	//kill the tank
					aliveTanks--;	//some tank was deleted
				}

			}

			if (avaliableTanks < 2) {
				//game over
				//show winner
				systemState = 50;
			}

			//game can continue
			//generate new map if there are less than 2 tanks
			if (aliveTanks <= 1) {
				for (int i = 0; i < tanksCount; i++) {
					//aliwe tank get some points
					if (tankAlive[i]) {
						tankPoints[i] += (tanksCount * 10);
					}
				}

				MakeNewMap();
			}

			//next tank and skip dead tanks
			do {
				activeTank++;
				if (activeTank >= tanksCount) {
					activeTank = 0;
				}
			} while (!tankAlive[activeTank]);

			missileType = 0;
			RedrawHid1();
			RedrawHid2();

		}

		break;
	case 20:
		//PREPARE GAME MENU
		DrawGameMenu();
		edgeBtnB = false;
		systemState++;
		break;
	case 21:
		//GAME MENU
		if (edgeBtnB) {
			RedrawScreen();
			RedrawBehindTanks();	//draw tanks (and small terain behind them)
			RedrawHid1();
			RedrawHid2();

			systemState = 10;	//go back to the game
		}

		if (edgeBtnLeft) {
			if (missileType > 0) {
				missileType--;
				for (int i = 0; i < 6; i++) {
					lcd.drawRect(24 + (i * 45), 154, 45, 60, ILI9341_DARKGREY);
					lcd.drawRect(25 + (i * 45), 155, 43, 58, ILI9341_DARKGREY);
				}
				lcd.drawRect(24 + (missileType * 45), 154, 45, 60, ILI9341_RED);
				lcd.drawRect(25 + (missileType * 45), 155, 43, 58, ILI9341_RED);
			}
		}

		if (edgeBtnRight) {
			if (missileType < 5) {
				missileType++;
				for (int i = 0; i < 6; i++) {
					lcd.drawRect(24 + (i * 45), 154, 45, 60, ILI9341_DARKGREY);
					lcd.drawRect(25 + (i * 45), 155, 43, 58, ILI9341_DARKGREY);
				}
				lcd.drawRect(24 + (missileType * 45), 154, 45, 60, ILI9341_RED);
				lcd.drawRect(25 + (missileType * 45), 155, 43, 58, ILI9341_RED);
			}
		}

		break;
	case 50:
		//GAME OVER
		if (tankAlive[0]) winner = 0;
		if (tankAlive[1]) winner = 1;
		if (tankAlive[2]) winner = 2;
		if (tankAlive[3]) winner = 3;

		lcd.fillRect(0, 0, 320, 240, ILI9341_BLACK);

		lcd.setTextSize(2);

		switch (winner) {
		case 0:
			lcd.setCursor(46, 10);
			winTankColor = TANK0_COLOR;
			lcd.setTextColor(winTankColor);
			lcd.print("! BLUE CANNON WIN !");
			break;
		case 1:
			lcd.setCursor(52, 10);
			winTankColor = TANK1_COLOR;
			lcd.setTextColor(winTankColor);
			lcd.print("! RED CANNON WIN !");
			break;
		case 2:
			lcd.setCursor(40, 10);
			winTankColor = TANK2_COLOR;
			lcd.setTextColor(winTankColor);
			lcd.print("! GREEN CANNON WIN !");
			break;
		case 3:
			lcd.setCursor(34, 10);
			winTankColor = TANK3_COLOR;
			lcd.setTextColor(winTankColor);
			lcd.print("! YELLOW CANNON WIN !");
			break;
		default:
			lcd.setCursor(40, 10);
			winTankColor = ILI9341_WHITE;
			lcd.setTextColor(winTankColor);
			lcd.print("! NOONE WIN !");
			break;
		}

		lcd.setTextColor(TANK0_COLOR);
		lcd.setTextSize(2);
		lcd.setCursor(10, 160);
		lcd.print("BLUE: ");
		lcd.print(tankPoints[0]);

		lcd.setTextColor(TANK1_COLOR);
		lcd.setCursor(170, 160);
		lcd.print("RED: ");
		lcd.print(tankPoints[1]);

		if (tanksCount > 2) {
			lcd.setTextColor(TANK2_COLOR);
			lcd.setCursor(10, 190);
			lcd.print("GREEN: ");
			lcd.print(tankPoints[2]);
		}

		if (tanksCount > 3) {
			lcd.setTextColor(TANK3_COLOR);
			lcd.setCursor(170, 190);
			lcd.print("YELLOW: ");
			lcd.print(tankPoints[3]);
		}

		lcd.setTextColor(ILI9341_DARKGREY);
		lcd.setCursor(5, 220);
		lcd.print("Press OK to continue...");

		systemState++;
		break;
	case 51:
		//ANIMATION
		GenerateEndScreen(animationAngle, winTankColor);

		animationAngle++;
		if (animationAngle >= 360) {
			animationAngle = 0;
		}

		delay(10);

		if (edgeBtnOk) {
			systemState = 0;	//go back to the menu
		}
		break;
	}

	//store last input states for edge detection
	lastBtnLeft = btnLeft;
	lastBtnOk = btnOk;
	lastBtnRight = btnRight;
	lastBtnUp = btnUp;
	lastBtnDown = btnDown;
	lastBtnA = btnA;
	lastBtnB = btnB;
}


void GenerateTerrain(int minWidth, int maxWidth, bool trn2Enbl) {
	int terrainPoints[11];

	//Reset old terrain and make random seeds
	for (int i = 0; i < 320; i++) {
		if (trn2Enbl) {
			terrain2[i] = 0;
		}
		else {
			terrain1[i] = 0;
		}
		
		if (i % 30 == 0) {
			terrainPoints[i / 30] = 240 - random(minWidth, maxWidth);
		}
	}


	//make bezier curve
	for (byte i = 0; i < 9; i += 2) {
		for (float j = 0; j < 1; j += 0.01) {
			int x1 = i * 32;
			int x2 = (i + 1) * 32;
			int x3 = (i + 2) * 32;

			int y1 = terrainPoints[i];
			int y2 = terrainPoints[i + 1];
			int y3 = terrainPoints[i + 2];

			int xa = GetPoint(x1, x2, j);
			int ya = GetPoint(y1, y2, j);
			int xb = GetPoint(x2, x3, j);
			int yb = GetPoint(y2, y3, j);

			int x = GetPoint(xa, xb, j);
			int y = GetPoint(ya, yb, j);
			
			if (trn2Enbl) {
				terrain2[x] = y;
			}
			else {
				terrain1[x] = y;
			}
		}
	}

 int GetPoint(int inX, int inY, float inPer) {
 //get bezier point for terrain generate
  int diff = inY - inX;
  return inX + (diff * inPer);
}

	//check and repair errors
	for (int i = 0; i < 320; i++) {
		if (trn2Enbl) {
			if (terrain2[i] == 0) {
				terrain2[i] = terrain2[i - 1];
			};

			//back terrain is lower than main terrain
			if ((terrain2[i] - 30) > terrain1[i]) {
				terrain2[i] = terrain1[i] + 30;
			}
		}
		else {
			if (terrain1[i] == 0) {
				terrain1[i] = terrain1[i - 1];
			}
		}	
	}
}



void MakeNewMap() {
	lcd.fillScreen(ILI9341_BLACK);	//sky

	lcd.setCursor(2, 2);
	lcd.setTextColor(ILI9341_WHITE);
	lcd.print("LOADING...");

	//moon
	lcd.fillCircle(305, 20, 18, 0x5280);
	lcd.fillCircle(305, 20, 15, ILI9341_YELLOW);
	lcd.fillCircle(random(280, 300), 20, 18, ILI9341_BLACK);

	//stars
	for (int i = 0; i < 25; i++) {
		//make some stars
		lcd.drawPixel(random(0, 319), random(0, 120), ILI9341_WHITE);
		lcd.drawPixel(random(0, 319), random(0, 120), ILI9341_DARKGREY);
	}

	//main terrain (main terrain generate first!!)
	randomSeed(analogRead(PA4) + activeTank);
	GenerateTerrain(20, 115, false);

	//back terrain
	randomSeed(analogRead(PA3) + activeTank);
	GenerateTerrain(45, 150, true);

	//possition of tanks
	//make tanks with some gap betwen them
	bool generateDone = false;
	while (!generateDone) {
		aliveTanks = tanksCount;	//all tanks are alive for next level

		//generate possition of tanks
		for (int i = 0; i < tanksCount; i++) {
			randomSeed(analogRead(PA3) + millis() + i);
			tankX[i] = random(10, 310);

			tankAngle[i] = 90;
			tankPower[i] = 50;
			
			if (tankPoints[i] > 0) {
				tankAlive[i] = true;
			}
			else {
				//kill tank without any points
				tankAlive[i] = false;	//kill the tank
				aliveTanks--;	//some tank was deleted
			}
		}

		int actualGap = 0;
		int smallestGap = 320;

		for (byte i = 0; i < tanksCount; i++) {
			for (byte j = 0; j < tanksCount; j++) {
				//check all combination
				if (i != j) {
					//test only diffetent tanks
					actualGap = abs(tankX[i] - tankX[j]);	//count distance
					if ((actualGap < smallestGap)) {
						smallestGap = actualGap;
					}
				}
			}
		}

		if (smallestGap > 30) {
			//smalest gap between two tanks is bigger than 30 pix.
			generateDone = true;
		}
	}

	RedrawScreen();
	RedrawBehindTanks();	//draw tanks (and small terain behind them)
	RedrawHid1();
	RedrawHid2();
	WindRecount();
}

void DrawTank(int posX, int posY, int colorCode, byte angle) {
	//posX = posX + 7; //offset for centering the gun
	//draw gun
	if (posY < 45 || posY > 240) {
		posY = 236;
	}

	int gunX = posX + 7 - cos((float)angle * DEG_TO_RAD) * 10.0;
	int gunY = posY - sin((float)angle * DEG_TO_RAD) * 10.0;
	lcd.drawLine(posX + 7, posY, gunX, gunY, ILI9341_WHITE);

	//draw body
	lcd.fillRect(posX, posY, 14, 4, colorCode);
	lcd.fillRect(posX + 1, posY - 2, 12, 2, ILI9341_BLACK);
	lcd.fillRect(posX + 2, posY - 3, 10, 1, ILI9341_BLACK);
	lcd.fillRect(posX + 3, posY - 4, 8, 1, ILI9341_WHITE);
}

void RedrawBehindTanks() {
	//make both terrains
	for (byte j = 0; j < tanksCount; j++) {
		for (int i = tankX[j] - 5; i < tankX[j] + 17; i++) {
			/*
			lcd.drawLine(i, terrain1[i], i, terrain1[i] - 20, 0x2945);
			lcd.drawLine(i, terrain1[i] + 35, i, terrain1[i], ILI9341_DARKGREEN);
			*/

			lcd.drawLine(i, terrain2[i] - 50, i, terrain2[i] - 65, ILI9341_BLACK);	//background
			lcd.drawLine(i, terrain1[i], i, terrain2[i] - 50, BACK_TERRAIN_COLOR);	//back terrain
			lcd.drawLine(i, terrain1[i] + 10, i, terrain1[i], ILI9341_DARKGREEN);	//main terrain
		}

		if (tankAlive[j]) {
			//draw only alive tanks
			tankY[j] = terrain1[tankX[j] + 7];

			if (tankY[j] < 4) {
				tankY[j] = 4;
			}

			int currentTankColor = TANK0_COLOR;	//color of active tank
			switch (j) {
			case 0:
				currentTankColor = TANK0_COLOR;
				break;
			case 1:
				currentTankColor = TANK1_COLOR;
				break;
			case 2:
				currentTankColor = TANK2_COLOR;
				break;
			case 3:
				currentTankColor = TANK3_COLOR;
				break;
			}

			DrawTank(tankX[j], tankY[j], currentTankColor, tankAngle[j]);
			
		}
	}

	RedrawHid2();	//redraw hid - change angle etc.
}

void RedrawScreen() {
	//Redraw all screen
	for (int i = 0; i < 320; i++) {
		lcd.drawLine(i, terrain1[i], i, terrain2[i] - 50, BACK_TERRAIN_COLOR);	//back terrain
		lcd.drawLine(i, 240, i, terrain1[i], ILI9341_DARKGREEN);	//main terrain
	}
}

void RedrawHid1() {
	lcd.fillRect(0, 0, tanksCount * 60, 12, ILI9341_BLACK);
	lcd.setTextSize(1);
	lcd.setCursor(2, 2);

	for (byte i = 0; i < tanksCount; i++) {

		switch (i) {
		case 0:
			lcd.setTextColor(TANK0_COLOR);
			if (!tankAlive[i]) {
				lcd.setTextColor(ILI9341_DARKGREY);
			}
			lcd.print("BE: ");
			break;
		case 1:
			lcd.setTextColor(TANK1_COLOR);
			if (!tankAlive[i]) {
				lcd.setTextColor(ILI9341_DARKGREY);
			}
			lcd.print("  RD: ");
			break;
		case 2:
			lcd.setTextColor(TANK2_COLOR);
			if (!tankAlive[i]) {
				lcd.setTextColor(ILI9341_DARKGREY);
			}
			lcd.print("  GN: ");
			break;
		case 3:
			lcd.setTextColor(TANK3_COLOR);
			if (!tankAlive[i]) {
				lcd.setTextColor(ILI9341_DARKGREY);
			}
			lcd.print("  YW: ");
			break;
		}

		lcd.print(tankPoints[i]);
	}
}

void RedrawHid2() {
	lcd.fillRect(0, 12, 225, 12, ILI9341_BLACK);
	lcd.setTextSize(1);
	lcd.setCursor(2, 14);
	
	switch (activeTank) {
	case 0:
		lcd.setTextColor(TANK0_COLOR);
		lcd.print("BLUE - PWR: ");
		break;
	case 1:
		lcd.setTextColor(TANK1_COLOR);
		lcd.print("RED - PWR: ");
		break;
	case 2:
		lcd.setTextColor(TANK2_COLOR);
		lcd.print("GREEN - PWR: ");
		break;
	case 3:
		lcd.setTextColor(TANK3_COLOR);
		lcd.print("YELLOW - PWR: ");
		break;
	}

	lcd.print(tankPower[activeTank]);
	lcd.print("  ANGLE: ");
	lcd.print(tankAngle[activeTank]);

	//draw missile picture
	lcd.fillRect(250, 12, 32, 11, ILI9341_BLACK);
	switch (missileType) {
	case 0:
		lcd.fillRect(261, 16, 5, 2, ILI9341_WHITE);
		lcd.fillRect(266, 15, 3, 4, ILI9341_WHITE);
		lcd.fillRect(269, 14, 3, 2, ILI9341_WHITE);
		lcd.fillRect(269, 18, 3, 2, ILI9341_WHITE);
		break;
	case 1:
		lcd.fillRect(260, 16, 1, 2, ILI9341_WHITE);
		lcd.fillRect(261, 15, 9, 4, ILI9341_WHITE);
		lcd.fillRect(266, 14, 6, 2, ILI9341_WHITE);
		lcd.fillRect(266, 18, 6, 2, ILI9341_WHITE);
		break;
	case 2:
		lcd.fillRect(260, 16, 10, 2, ILI9341_WHITE);
		lcd.fillRect(261, 15, 7, 4, ILI9341_WHITE);
		lcd.fillRect(262, 14, 5, 6, ILI9341_WHITE);
		lcd.drawLine(269, 15, 272, 15, ILI9341_WHITE);
		lcd.drawLine(269, 18, 272, 18, ILI9341_WHITE);
		break;
	case 3:
		lcd.fillRect(260, 14, 9, 2, ILI9341_WHITE);
		lcd.drawLine(267, 13, 272, 13, ILI9341_WHITE);
		lcd.drawLine(267, 16, 272, 16, ILI9341_WHITE);
		for (int i = 260; i < 272; i += 2) {
			lcd.drawPixel(i + 1, 18, ILI9341_WHITE);
			lcd.drawPixel(i, 19, ILI9341_WHITE);
		}
		break;
	case 4:
		lcd.drawLine(264, 13, 267, 13, ILI9341_WHITE);
		lcd.drawPixel(263, 14, ILI9341_WHITE);
		lcd.drawPixel(262, 15, ILI9341_WHITE);
		lcd.drawPixel(263, 16, ILI9341_WHITE);
		lcd.drawPixel(264, 17, ILI9341_WHITE);
		lcd.drawPixel(265, 18, ILI9341_WHITE);
		lcd.drawPixel(266, 18, ILI9341_WHITE);
		lcd.drawPixel(267, 17, ILI9341_WHITE);
		lcd.drawPixel(268, 16, ILI9341_WHITE);
		lcd.drawPixel(269, 15, ILI9341_WHITE);
		lcd.drawPixel(268, 14, ILI9341_WHITE);
		lcd.fillRect(264, 19, 4, 3, ILI9341_WHITE);
		break;
	case 5:
		lcd.drawLine(260, 14, 260, 21, ILI9341_WHITE);
		lcd.drawLine(268, 14, 268, 21, ILI9341_WHITE);
		lcd.drawLine(262, 14, 262, 19, ILI9341_WHITE);
		lcd.drawLine(266, 16, 266, 19, ILI9341_WHITE);
		lcd.drawLine(264, 16, 264, 17, ILI9341_WHITE);

		lcd.drawLine(260, 21, 268, 21, ILI9341_WHITE);
		lcd.drawLine(262, 14, 268, 14, ILI9341_WHITE);
		lcd.drawLine(262, 19, 266, 19, ILI9341_WHITE);
		lcd.drawPixel(265, 16, ILI9341_WHITE);
		break;
	}
}

void DamageTerrain(int posX, byte mt) {
	//in case out of screen
	if (posX < 0 || posX > 319) {
		return;
	}

	//generate and draw new terrain
	int rightLimit = 10;
	int leftLimit = -10;
	int maxDepth = 10;

	switch (mt) {
	case 0:
		rightLimit = 10;
		leftLimit = -10;
		maxDepth = 10;
		break;
	case 1:
		rightLimit = 15;
		leftLimit = -15;
		maxDepth = 15;
		break;
	case 2:
		rightLimit = 25;
		leftLimit = -25;
		maxDepth = 30;
		break;
	case 3:
		rightLimit = 50;
		leftLimit = -50;
		maxDepth = 15;
		break;
	case 4:
		rightLimit = 10;
		leftLimit = -10;
		maxDepth = 15;
		break;
	case 5:
		rightLimit = 45;
		leftLimit = -45;
		maxDepth = 45;
		break;
	case 10:
		rightLimit = 10;
		leftLimit = -5;
		maxDepth = 15;
		break;
	}
	

	for (int i = leftLimit; i < rightLimit; i++) {
		int terrainX = posX + i;
		int terrainDown = cos(abs(i) * DEG_TO_RAD * 90 / rightLimit) * maxDepth;

		if ((terrainX >= 0) && (terrainX < 320)) {
			//draw old back terrain
			lcd.drawLine(terrainX, terrain2[terrainX]-50, terrainX, terrain2[terrainX] - 60, ILI9341_BLACK);	//background
			lcd.drawLine(terrainX, terrain1[terrainX], terrainX, terrain2[terrainX]-50, BACK_TERRAIN_COLOR);	//back terrain
			lcd.drawLine(terrainX, 240, terrainX, terrain1[terrainX], ILI9341_DARKGREEN);	//main terrain

			//update olny real terrain
			terrain1[terrainX] += terrainDown;

			if (terrain1[terrainX] > 240 || terrain1[terrainX] < 75) {
				//overflow detected
				terrain1[terrainX] = 240;
			}

			lcd.drawLine(terrainX, terrain1[terrainX], terrainX, terrain1[terrainX] - terrainDown, BACK_TERRAIN_COLOR);	//back terrain
			lcd.drawLine(terrainX, 240, terrainX, terrain1[terrainX], ILI9341_DARKGREEN);	//main terrain
		}
	}
}

void WindRecount() {
	//generate new power of wind
	wind = random(-9, 9);
	//wind = 0;

	//show arrows
	lcd.fillRect(250, 0, 32, 12, ILI9341_BLACK);
	lcd.setTextColor(ILI9341_CYAN);

	if (wind < -7) {
		lcd.fillTriangle(257, 2, 257, 9, 253, 5, ILI9341_CYAN);
	}

	if (wind < -4) {
		lcd.fillTriangle(261, 2, 261, 9, 257, 5, ILI9341_CYAN);
	}

	if (wind < -1) {
		lcd.fillTriangle(265, 2, 265, 9, 261, 5, ILI9341_CYAN);
	}


	if (wind > 1) {
		lcd.fillTriangle(267, 2, 267, 9, 271, 5, ILI9341_CYAN);
	}

	if (wind > 4) {
		lcd.fillTriangle(271, 2, 271, 9, 275, 5, ILI9341_CYAN);
	}

	if (wind > 7) {
		lcd.fillTriangle(275, 2, 275, 9, 279, 5, ILI9341_CYAN);
	}
}

void GenerateSplashScreen() {
	lcd.fillRect(0, 0, 320, 240, ILI9341_BLACK);

	//stars
	for (int i = 0; i < 25; i++) {
		//make some stars
		lcd.drawPixel(random(0, 319), random(0, 120), ILI9341_WHITE);
		lcd.drawPixel(random(0, 319), random(0, 120), ILI9341_DARKGREY);
	}

	//moon
	lcd.fillCircle(305, 20, 18, 0x5280);
	lcd.fillCircle(305, 20, 15, ILI9341_YELLOW);
	lcd.fillCircle(random(280, 300), 20, 18, ILI9341_BLACK);

	//generate background terrain
	GenerateTerrain(45, 100, false);	//main teren generate first!!
	GenerateTerrain(45, 200, true);
	RedrawScreen();

	//write texts
	lcd.setTextColor(ILI9341_CYAN);
	lcd.setTextSize(4);
	lcd.setCursor(10, 10);
	lcd.print("CANNONS IN");
	lcd.setCursor(50, 50);
	lcd.print("THE CANYON");

	lcd.setTextSize(1);
	lcd.setCursor(280, 80);
	lcd.print("v.");
	lcd.print(appVersion);
	lcd.print(".");
	lcd.print(appRevision);
	lcd.setCursor(280, 90);
	lcd.print("190615");

	lcd.setTextColor(ILI9341_BLACK);
	lcd.setTextSize(2);
	lcd.setCursor(110, 220);
	lcd.print("Press OK to start");

	lcd.setTextColor(ILI9341_WHITE);
	lcd.setCursor(35, 90);
	lcd.print("by mylms.cz");

	//DrawCannons
	//barrels
	for (int i = 0; i < 10; i++) {
		lcd.drawLine(80 + i, 170 + i, 125 + i, 140 + i, ILI9341_WHITE);
		lcd.drawLine(280 - i, 140 + i, 245 - i, 100 + i, ILI9341_WHITE);
	}

	//canon 1
	lcd.fillCircle(66, 191, 33, ILI9341_WHITE);	//dome
	lcd.fillRoundRect(26, 189, 80, 38, 4, ILI9341_RED);	//base

	//cannon 2
	lcd.fillCircle(266, 161, 33, ILI9341_WHITE);	//dome
	lcd.fillRoundRect(226, 159, 80, 38, 4, ILI9341_BLUE);	//base

	//text "RED" and "BLUE" on canons
	lcd.setTextColor(ILI9341_DARKGREY);
	lcd.setCursor(48, 200);
	lcd.print("RED");
	lcd.setCursor(242, 170);
	lcd.print("BLUE");

	lcd.setTextColor(ILI9341_BLACK);
	lcd.setCursor(49, 201);
	lcd.print("RED");
	lcd.setCursor(243, 171);
	lcd.print("BLUE");
}

void DrawMenu() {
	lcd.fillRect(0, 0, 320, 240, ILI9341_BLACK);

	//draw main text
	lcd.setTextColor(ILI9341_CYAN);
	lcd.setTextSize(2);
	lcd.setCursor(5, 5);
	lcd.print("Cannons in the canyon");
	lcd.setTextSize(1);
	lcd.setCursor(215, 25);
	lcd.print("see mylms.cz/cic");

	//buttons
	lcd.fillRect(196, 40, 24, 24, ILI9341_WHITE);
	lcd.fillRect(196, 98, 24, 24, ILI9341_WHITE);
	lcd.fillRect(257, 69, 24, 24, ILI9341_WHITE);
	lcd.fillTriangle(254, 63, 283, 63, 268, 49, ILI9341_WHITE);
	lcd.fillTriangle(251, 66, 251, 95, 237, 81, ILI9341_WHITE);
	lcd.fillTriangle(254, 98, 283, 98, 268, 112, ILI9341_WHITE);
	lcd.fillTriangle(286, 66, 286, 95, 300, 81, ILI9341_WHITE);

	//texts in buttons
	lcd.setTextColor(ILI9341_BLACK);
	lcd.setCursor(205, 48); //+9, +8
	lcd.print("A");
	lcd.setCursor(205, 106);
	lcd.print("B");
	lcd.setCursor(263, 77);
	lcd.print("OK");
	lcd.setCursor(266, 55);
	lcd.print("U");
	lcd.setCursor(245, 77);
	lcd.print("L");
	lcd.setCursor(266, 100);
	lcd.print("D");
	lcd.setCursor(288, 77);
	lcd.print("R");

	lcd.setTextColor(ILI9341_WHITE);
	lcd.setCursor(195, 140);
	lcd.print("A = GAME MENU");
	lcd.setCursor(195, 155);
	lcd.print("B = START GAME");
	lcd.setCursor(195, 180);
	lcd.print("OK = FIRE!");
	lcd.setCursor(195, 195);
	lcd.print("L, R = ANGLE");
	lcd.setCursor(195, 210);
	lcd.print("U, D = POWER");
	lcd.setCursor(10, 210);
	lcd.print("Press 'B' to start game...");

	lcd.drawLine(180, 40, 180, 230, ILI9341_WHITE);	//line separator
	lcd.drawLine(181, 40, 181, 230, ILI9341_WHITE);	//line separator

	//draw tanks
	DrawTank(15, 55, TANK0_COLOR, 45);
	DrawTank(55, 55, TANK1_COLOR, 45);
	DrawTank(95, 55, TANK2_COLOR, 45);
	DrawTank(135, 55, TANK3_COLOR, 45);

	//tanks count selector (2 tanks)
	lcd.drawRect(5, 40, 80, 24, ILI9341_WHITE);
}

void GenerateEndScreen(int angle, int color) {
	int offsetX = 160;
	int offsetY = 80;

	lcd.fillRect(80, 40, 161, 106, ILI9341_BLACK);

	float ref1x = sin(angle * DEG_TO_RAD);
	float ref1y = cos(angle * DEG_TO_RAD);
	float ref2x = sin((angle + 90) * DEG_TO_RAD);
	float ref2y = cos((angle + 90) * DEG_TO_RAD);
	float ref3x = sin((angle + 180) * DEG_TO_RAD);
	float ref3y = cos((angle + 180) * DEG_TO_RAD);
	float ref4x = sin((angle + 270) * DEG_TO_RAD);
	float ref4y = cos((angle + 270) * DEG_TO_RAD);
	

	int ax1 = offsetX + (ref1x * 80);
	int ay1 = offsetY + (ref1y * 40);

	int ax2 = offsetX + (ref2x * 80);
	int ay2 = offsetY + (ref2y * 40);

	int ax3 = offsetX + (ref3x * 80);
	int ay3 = offsetY + (ref3y * 40);

	int ax4 = offsetX + (ref4x * 80);
	int ay4 = offsetY + (ref4y * 40);


	int bx1 = offsetX + (ref1x * 60);
	int by1 = offsetY + (ref1y * 30);

	int bx2 = offsetX + (ref2x * 60);
	int by2 = offsetY + (ref2y * 30);

	int bx3 = offsetX + (ref3x * 60);
	int by3 = offsetY + (ref3y * 30);

	int bx4 = offsetX + (ref4x * 60);
	int by4 = offsetY + (ref4y * 30);


	int cx1 = offsetX + (ref1x * 40);
	int cy1 = offsetY + (ref1y * 20);

	int cx2 = offsetX + (ref2x * 40);
	int cy2 = offsetY + (ref2y * 20);

	int cx3 = offsetX + (ref3x * 40);
	int cy3 = offsetY + (ref3y * 20);

	int cx4 = offsetX + (ref4x * 40);
	int cy4 = offsetY + (ref4y * 20);

	//main square
	lcd.drawLine(ax1, ay1, ax2, ay2, color);
	lcd.drawLine(ax2, ay2, ax3, ay3, color);
	lcd.drawLine(ax3, ay3, ax4, ay4, color);
	lcd.drawLine(ax4, ay4, ax1, ay1, color);

	//down square
	if (ax1 < ax2) lcd.drawLine(ax1, ay1 + 25, ax2, ay2 + 25, color);
	if (ax2 < ax3) lcd.drawLine(ax2, ay2 + 25, ax3, ay3 + 25, color);
	if (ax3 < ax4) lcd.drawLine(ax3, ay3 + 25, ax4, ay4 + 25, color);
	if (ax4 < ax1) lcd.drawLine(ax4, ay4 + 25, ax1, ay1 + 25, color);

	//vertical lines
	if (ax4 < ax1 || ax1 < ax2) lcd.drawLine(ax1, ay1, ax1, ay1 + 25, color);
	if (ax1 < ax2 || ax2 < ax3) lcd.drawLine(ax2, ay2, ax2, ay2 + 25, color);
	if (ax2 < ax3 || ax3 < ax4) lcd.drawLine(ax3, ay3, ax3, ay3 + 25, color);
	if (ax3 < ax4 || ax4 < ax1) lcd.drawLine(ax4, ay4, ax4, ay4 + 25, color);

	//2 square
	lcd.drawLine(ax1, ay1, bx1, by1, color);
	lcd.drawLine(ax2, ay2, bx2, by2, color);
	lcd.drawLine(ax3, ay3, bx3, by3, color);
	lcd.drawLine(ax4, ay4, bx4, by4, color);

	//
	lcd.drawLine(bx1, by1, bx1, by1 - 10, color);
	lcd.drawLine(bx2, by2, bx2, by2 - 10, color);
	lcd.drawLine(bx3, by3, bx3, by3 - 10, color);
	lcd.drawLine(bx4, by4, bx4, by4 - 10, color);

	lcd.drawLine(bx1, by1 - 10, bx2, by2 - 10, color);
	lcd.drawLine(bx2, by2 - 10, bx3, by3 - 10, color);
	lcd.drawLine(bx3, by3 - 10, bx4, by4 - 10, color);
	lcd.drawLine(bx4, by4 - 10, bx1, by1 - 10, color);


	lcd.drawLine(bx1, by1 - 10, cx1, cy1 - 10, color);
	lcd.drawLine(bx2, by2 - 10, cx2, cy2 - 10, color);
	lcd.drawLine(bx3, by3 - 10, cx3, cy3 - 10, color);
	lcd.drawLine(bx4, by4 - 10, cx4, cy4 - 10, color);

	lcd.drawLine(cx1, cy1 - 10, cx1, cy1 - 20, color);
	lcd.drawLine(cx2, cy2 - 10, cx2, cy2 - 20, color);
	lcd.drawLine(cx3, cy3 - 10, cx3, cy3 - 20, color);
	lcd.drawLine(cx4, cy4 - 10, cx4, cy4 - 20, color);

	lcd.drawLine(cx1, cy1 - 20, cx2, cy2 - 20, color);
	lcd.drawLine(cx2, cy2 - 20, cx3, cy3 - 20, color);
	lcd.drawLine(cx3, cy3 - 20, cx4, cy4 - 20, color);
	lcd.drawLine(cx4, cy4 - 20, cx1, cy1 - 20, color);
}

void DrawGameMenu() {
	//background
	//lcd.fillRect(9, 139, 300, 10, ILI9341_BLACK);	//top
	//lcd.fillRect(9, 139, 10, 90, ILI9341_BLACK);	//l
	//lcd.fillRect(299, 139, 10, 90, ILI9341_BLACK);	//r
	//lcd.fillRect(9, 219, 300, 10, ILI9341_BLACK);	//dn

	//lcd.fillRect(19, 149, 280, 70, ILI9341_DARKGREY);	//back


	for (int i = 0; i < 6; i++) {
		lcd.fillRect(24 + (i * 45), 154, 45, 60, ILI9341_WHITE);
		
		lcd.drawRect(24 + (i * 45), 154, 45, 60, ILI9341_DARKGREY);
		lcd.drawRect(25 + (i * 45), 155, 43, 58, ILI9341_DARKGREY);

		int offX = 40 + (i * 45);
		int offY = 157;
		lcd.setTextColor(ILI9341_BLACK);

		switch (i) {
		case 0:
			lcd.fillRect(offX + 1, offY+6, 5, 2, ILI9341_BLACK);
			lcd.fillRect(offX +6, offY+5, 3, 4, ILI9341_BLACK);
			lcd.fillRect(offX+9, offY+4, 3, 2, ILI9341_BLACK);
			lcd.fillRect(offX+9, offY+8, 3, 2, ILI9341_BLACK);

			lcd.setCursor(offX - 10, offY + 15);
			lcd.print("$");
			lcd.print(MISSILE0_PRICE);

			lcd.setCursor(offX - 10, offY + 25);
			lcd.print("WD 10");

			lcd.setCursor(offX - 10, offY + 35);
			lcd.print("DP 10");

			lcd.setCursor(offX - 10, offY + 45);
			lcd.print("PWR 10");
			break;
		case 1:
			lcd.fillRect(offX, offY+6, 1, 2, ILI9341_BLACK);
			lcd.fillRect(offX+1, offY+5, 9, 4, ILI9341_BLACK);
			lcd.fillRect(offX+6, offY+4, 6, 2, ILI9341_BLACK);
			lcd.fillRect(offX+6, offY+8, 6, 2, ILI9341_BLACK);

			lcd.setCursor(offX - 10, offY + 15);
			lcd.print("$");
			lcd.print(MISSILE1_PRICE);

			lcd.setCursor(offX - 10, offY + 25);
			lcd.print("WD 15");

			lcd.setCursor(offX - 10, offY + 35);
			lcd.print("DP 15");

			lcd.setCursor(offX - 10, offY + 45);
			lcd.print("PWR 15");

			break;
		case 2:
			lcd.fillRect(offX, offY+6, 10, 2, ILI9341_BLACK);
			lcd.fillRect(offX+1, offY+5, 7, 4, ILI9341_BLACK);
			lcd.fillRect(offX+2, offY+4, 5, 6, ILI9341_BLACK);
			lcd.drawLine(offX+9, offY+5, offX+12, offY+5, ILI9341_BLACK);
			lcd.drawLine(offX+9, offY+8, offX+12, offY+8, ILI9341_BLACK);

			lcd.setCursor(offX - 10, offY + 15);
			lcd.print("$");
			lcd.print(MISSILE2_PRICE);

			lcd.setCursor(offX - 10, offY + 25);
			lcd.print("WD 25");

			lcd.setCursor(offX - 10, offY + 35);
			lcd.print("DP 30");

			lcd.setCursor(offX - 10, offY + 45);
			lcd.print("PWR 25");
			break;
		case 3:
			lcd.fillRect(offX, offY+4, 9, 2, ILI9341_BLACK);
			lcd.drawLine(offX+7, offY+3, offX+12, offY+3, ILI9341_BLACK);
			lcd.drawLine(offX+7, offY+6, offX+12, offY+6, ILI9341_BLACK);
			for (int j = offX; j < offX+12; j += 2) {
				lcd.drawPixel(j + 1, offY+8, ILI9341_BLACK);
				lcd.drawPixel(j, offY+9, ILI9341_BLACK);
			}

			lcd.setCursor(offX - 10, offY + 15);
			lcd.print("$");
			lcd.print(MISSILE3_PRICE);

			lcd.setCursor(offX - 10, offY + 25);
			lcd.print("WD 50");

			lcd.setCursor(offX - 10, offY + 35);
			lcd.print("DP 15");

			lcd.setCursor(offX - 10, offY + 45);
			lcd.print("PWR 50");
			break;
		case 4:
			lcd.drawLine(offX+4, offY+3, offX+7, offY+3, ILI9341_BLACK);
			lcd.drawPixel(offX+3, offY+4, ILI9341_BLACK);
			lcd.drawPixel(offX+2, offY+5, ILI9341_BLACK);
			lcd.drawPixel(offX+3, offY+6, ILI9341_BLACK);
			lcd.drawPixel(offX+4, offY+7, ILI9341_BLACK);
			lcd.drawPixel(offX+5, offY+8, ILI9341_BLACK);
			lcd.drawPixel(offX+6, offY+8, ILI9341_BLACK);
			lcd.drawPixel(offX+7, offY+7, ILI9341_BLACK);
			lcd.drawPixel(offX+8, offY+6, ILI9341_BLACK);
			lcd.drawPixel(offX+9, offY+5, ILI9341_BLACK);
			lcd.drawPixel(offX+8, offY+4, ILI9341_BLACK);
			lcd.fillRect(offX+4, offY+9, 4, 3, ILI9341_BLACK);

			lcd.setCursor(offX - 10, offY + 15);
			lcd.print("$");
			lcd.print(MISSILE4_PRICE);

			lcd.setCursor(offX - 10, offY + 25);
			lcd.print("WD 10");

			lcd.setCursor(offX - 10, offY + 35);
			lcd.print("DP 15");

			lcd.setCursor(offX - 10, offY + 45);
			lcd.print("PWR 10");
			break;
		case 5:
			lcd.drawLine(offX, offY+4, offX, offY+11, ILI9341_BLACK);
			lcd.drawLine(offX+8, offY+4, offX+8, offY+11, ILI9341_BLACK);
			lcd.drawLine(offX+2, offY+4, offX+2, offY+9, ILI9341_BLACK);
			lcd.drawLine(offX+6, offY+6, offX+6, offY+9, ILI9341_BLACK);
			lcd.drawLine(offX+4, offY+6, offX+4, offY+7, ILI9341_BLACK);

			lcd.drawLine(offX, offY+11, offX+8, offY+11, ILI9341_BLACK);
			lcd.drawLine(offX+2, offY+4, offX+8, offY+4, ILI9341_BLACK);
			lcd.drawLine(offX+2, offY+9, offX+6, offY+9, ILI9341_BLACK);
			lcd.drawPixel(offX+5, offY+6, ILI9341_BLACK);

			lcd.setCursor(offX - 10, offY + 15);
			lcd.print("$");
			lcd.print(MISSILE5_PRICE);

			lcd.setCursor(offX - 10, offY + 25);
			lcd.print("WD 40");

			lcd.setCursor(offX - 10, offY + 35);
			lcd.print("DP 45");

			lcd.setCursor(offX - 10, offY + 45);
			lcd.print("PWR 10");
			break;
		}
	}

	//lcd.setCursor

	lcd.drawRect(24 + (missileType * 45), 154, 45, 60, ILI9341_RED);
	lcd.drawRect(25 + (missileType * 45), 155, 43, 58, ILI9341_RED);

	lcd.setTextSize(1);
}
