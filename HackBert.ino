#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)
#define DREQ          3      // VS1053 Data request, ideally an Interrupt pin
#define CARDCS        4      // Card chip select pin
Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

// VS1053 play speed parameter
#define para_playSpeed 0x1E04

// Enable or disable debugging via serial monitor here
//#define DEBUG

// constants won't change

// the number of the pin that is used for the pushbuttons
const int buttonsPin = A0;

// the pin of the potentiometer that is used to control the volume
const int volumePin = A1;

// wait before next click is recognized
const int buttonPressedDelay = 1000;


// variables will change

byte currentFolder = 1;
unsigned int currentFile = 0;
unsigned int numberOfFiles[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// the current volume level, set to min at start
byte volumeState = 254;

// last button that was pressed
byte lastPressedButton = 0;
// is the last pressed button released
boolean released = true;
// remember if the back button was pressed last time
byte lastReleasedButton = 0;
// the time at the back button was pressed last time
long lastBackButtonTime = 0;

char currentTrackFileName[] = "/current.txt";

byte pressedButton = 0;

// the setup routine runs once when you turn the device on or you press reset
void setup()
{
  // disable LED L
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

#if defined DEBUG
  // initialize serial communication at 9600 bits per second
  Serial.begin(9600);
#endif

  // initialise the music player
  if (!musicPlayer.begin())
  {
    //Serial.println("VS1053 not found");
    while (1);  // don't do anything more
  }

  // initialise the SD card
  SD.begin(CARDCS);


  // If DREQ is on an interrupt pin (on uno, #2 or #3) we can do background
  // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int

  // read the number of tracks in each folder
  for (byte i = 0; i < 10; i++)
  {
    String temp = "/";
    temp.concat(i);
    char filename[3];
    temp.toCharArray(filename, sizeof(filename));
    numberOfFiles[i] = countFiles(SD.open(filename));
#if defined DEBUG
    Serial.print(filename);
    Serial.print(": ");
    Serial.println(numberOfFiles[i]);
#endif
  }

  // read remembered track
  if (SD.exists(currentTrackFileName))
  {
    File file = SD.open(currentTrackFileName, FILE_READ);
    if (file)
    {
      currentFolder = file.readStringUntil('\n').toInt();
      currentFile = file.readStringUntil('\n').toInt() - 1;
    }
    file.close();
  }


  delay(100); // init delay
}


// counts the number of files in directory
unsigned int countFiles(File dir)
{
  unsigned int counter = 0;
  while (true)
  {
    File entry = dir.openNextFile();
    if (!entry)
    {
      // no more files
      break;
    }

    counter++;
    entry.close();
  }
  dir.close();

  return counter;
}

// the loop routine runs over and over again forever
void loop()
{
  if (musicPlayer.stopped() && pressedButton == 11)
  {
#if defined DEBUG
    Serial.println("Playlist Song gestoppt");
#endif
    currentFolder = 2;
    playNext();
  }
  // play next song if player stopped
  if (musicPlayer.stopped() && pressedButton != 11)
  {
    playNext();
  }

  // check the volume and set it
  checkVolume();

  // check if a button is pressed and perform some action
  checkButtons();

  delay(1); // delay in between reads for stability
}


// checks the value of the potentiometer
// if it has changed by 2 then set the new volume
void checkVolume()
{
  // read the state of the volume potentiometer
  int read = analogRead(volumePin);

  // set the range of the volume from max=0 to min=254
  // (limit max volume to 20 and min to 60)
  byte state = map(read, 0, 1023, 10, 90);


  // recognize state (volume) changes in steps of two
  if (state < volumeState - 1 || state > volumeState + 1)
  {
    // remember the new volume state
    volumeState = state;

    // set volume max=0, min=254
    musicPlayer.setVolume(volumeState, 254);

    // print out the state of the volume
#if defined DEBUG
    Serial.print(volumePin);
    Serial.print(" volume ");
    Serial.println(100-volumeState);
#endif
  }
}

// check if some button is pressed
// play first track, if button is not pressed last time
// play next track, if a button is pressed again
void checkButtons()
{
  // get the pressed button
  pressedButton = getPressedButton();

  // if a button is pressed
  if (pressedButton != 0)
  {
#if defined DEBUG
    Serial.print("Taste: ");
    Serial.println(pressedButton);
#endif

    // if a track/play list button is pressed
    if (pressedButton < 10 && released)
    {
      musicPlayer.stopPlaying();
      if (currentFolder == pressedButton)
      {
        playNext();
      }
      else
      {
        currentFolder = pressedButton;
        currentFile = 1;
        playCurrent();
      }

    }
    // if a function button is pressed
    else
    {
      if (pressedButton == 10 && released)
      {
        musicPlayer.stopPlaying();
        long time = millis();

        // this is the second press within 1 sec., so we
        // got to the previous track
        if (lastReleasedButton == 10 &&
            ((time - lastBackButtonTime) < buttonPressedDelay))
        {
          playPrevious();
        }
        else
        {
          playCurrent();
        }
        lastBackButtonTime = time;
      }
      else if (pressedButton == 11 && released)
      {
        // play all tracks in all folders
        musicPlayer.stopPlaying();
        currentFolder = 1;
        currentFile = 1;
        playCurrent();
      }
    }

    released = false;
    lastReleasedButton = pressedButton;
  }
  else
  {
    released = true;

    // reset play speed
    if (lastPressedButton == 11)
    {
#if defined DEBUG
      Serial.println("normal speed");
#endif
      musicPlayer.sciWrite(VS1053_REG_WRAMADDR, para_playSpeed);
      musicPlayer.sciWrite(VS1053_REG_WRAM, 1);
    }
  }

  // remember pressed button
  lastPressedButton = pressedButton;
}


void playPrevious()
{
  currentFile--;
  if (currentFile < 1)
  {
    currentFile = numberOfFiles[currentFolder];
  }
  playCurrent();
}

void playNext()
{
  currentFile++;
  if (currentFile > numberOfFiles[currentFolder])
  {
    currentFile = 1;
  }
  playCurrent();
}

void playCurrent()
{
  if (numberOfFiles[currentFolder] > 0)
  {
    rememberCurrentTrack();

    String temp = "/";
    temp.concat(currentFolder);
    temp.concat("/");
    temp.concat(currentFile);
    temp.concat(".wav");
    char filename[temp.length() + 1];
    temp.toCharArray(filename, sizeof(filename));
    musicPlayer.startPlayingFile(filename);
#if defined DEBUG
    Serial.print("Play ");
    Serial.println(filename);
#endif
  }
}

void rememberCurrentTrack()
{
  if (SD.exists(currentTrackFileName))
  {
    SD.remove(currentTrackFileName);
  }

  File file = SD.open(currentTrackFileName, FILE_WRITE);
  if (file)
  {
    file.println(currentFolder);
    file.println(currentFile);
  }
  file.close();
}


// returns 0 if no button is pressed,
// else the number of the pressed button is returned (1 - 11)
byte getPressedButton()
{
  int buttonsPinValue = analogRead(buttonsPin);
  byte pressedButton = 0;

  if (buttonsPinValue > 823)
  {
    // button 6 has a value of about 878
    pressedButton = 6;
  }
  else if (buttonsPinValue > 725)
  {
    // button 5 has a value of about 768
    pressedButton = 5;
  }
  else if (buttonsPinValue > 649)
  {
    // button 4 has a value of about 683
    pressedButton = 4;
  }
  else if (buttonsPinValue > 586)
  {
    // button 1 has a value of about 614
    pressedButton = 1;
  }
  else if (buttonsPinValue > 535)
  {
    // button 2 has a value of about 559
    pressedButton = 2;
  }
  else if (buttonsPinValue > 492)
  {
    // button 3 has a value of about 512
    pressedButton = 3;
  }
  else if (buttonsPinValue > 450)
  {
    // if no button is pressed the value is of about 473
    pressedButton = 0;
  }
  else if (buttonsPinValue > 400)
  {
    // button 11 has a value of about 427
    pressedButton = 11;
  }
  else if (buttonsPinValue > 340)
  {
    // button 10 has a value of about 372
    pressedButton = 10;
  }
  else if (buttonsPinValue > 267)
  {
    // button 7 has a value of about 307
    pressedButton = 7;
  }
  else if (buttonsPinValue > 178)
  {
    // button 8 has a value of about 228
    pressedButton = 8;
  }
  else if (buttonsPinValue > 0)
  {
    // button 9 has a value of about 128
    pressedButton = 9;
  }
  return pressedButton;
}
