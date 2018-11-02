#include <FastLED.h>
#include <SimbleeForMobile.h>

//
//  Get Simblee to work with the most basic program
//
//  6/11/18
//
//  ToDo: Read about call back in the Simblee documentation
//
//  Incorporate this into linear lights regular, then fish regular
//  Library?
//
#define BKGND_COLOR  85,85,85  // r,g,b
#define TEXT_COLOR  WHITE
#define NORMAL_TEXT_SIZE  18
#define LRG_TEXT_SIZE     24
#define FULL_WIDTH  525
#define BORDER  10

uint8_t MAX_SHOW = 12;

volatile bool needsUpdate;
uint8_t eventId;

// Widget identifiers (uint8_t)
uint8_t brightness_slider, brightness_textfield;
uint8_t spd_slider, spd_textfield;
uint8_t forecolor_slider, forecolor_swatch;
uint8_t backcolor_slider, backcolor_swatch;
uint8_t show_stepper, show_textfield;
uint8_t on_switch, noise_switch, palette_switch;
uint8_t palette_center_text, palette_center_slider, palette_center_swatch;
uint8_t palette_width_text, palette_width_slider, palette_width_textfield;

#define NUM_BARS  20  // How many color bars for the static color bars
#define palette_swatch_number  20  // How many bars for the dynamic palette swatch
uint8_t palette_swatch[palette_swatch_number];
uint8_t palette_center_rects[NUM_BARS];

// Variables
uint8_t brightness = 255;
uint8_t spd = 10;
uint8_t forecolor = 0;
uint8_t backcolor = 128;
uint8_t current_show = 0;
bool is_on, has_noise, do_palette;
uint8_t palette_center = 0;
uint8_t palette_width = 255;

void setup() {
  Serial.begin(9600);
  // Serial.begin(speed, RXpin, TXpin);  // This will reassign the serial RX & TX pins, possibly useful for XBee
  // If the pins are reassigned, they may need pinMode (RXpin, OUTPUT); pinMode (RXpin, OUTPUT);
  
  // ToDo: Check XBee for I2C, as Simblee does support Simblee as Master
  // Commands will looks like Wire.begin() and Wire.available();
  // Currently, xBee works just therough Serial.print(message);
  Serial.println("Simble Test started");

  // These pinModes may be necessary
  #define DATA_PIN 9
  #define CLOCK_PIN 8
  pinMode (DATA_PIN, OUTPUT);
  pinMode (CLOCK_PIN, OUTPUT);
  
  SimbleeForMobile.deviceName = "Lights";  // Big name
  SimbleeForMobile.advertisementData = "Fish";  // Small name
  SimbleeForMobile.begin();
}

void loop() {
  if(needsUpdate) {
    needsUpdate = false;
    update();
  }
  SimbleeForMobile.process();  // end loop with this Simble function
}

//
//  ui - define buttons and sliders
//
void ui() {

  // iPhone 6 is (320 x 568)
  
  color_t bkgnd_color = rgb(BKGND_COLOR);
  
  // Background
  SimbleeForMobile.beginScreen(bkgnd_color);

  // Top text
  SimbleeForMobile.drawText(90, 25, "LED Controller", TEXT_COLOR, LRG_TEXT_SIZE);

  // Brightness slider - 40 vertical pixels between sliders
  SimbleeForMobile.drawText(10, 70, "Brightness", TEXT_COLOR);  // left_start, vert_position, text, color
  brightness_slider = SimbleeForMobile.drawSlider(95, 65, 155, 0, 255); // left_start, vert_pos, width, min, max
  brightness_textfield = SimbleeForMobile.drawTextField(255, 65, 40, brightness, "", TEXT_COLOR, bkgnd_color);  // left_start, vert_pos, width, start_value
  updateBrightness();
    
  // Speed slider
  SimbleeForMobile.drawText(30, 110, "Speed", TEXT_COLOR);
  spd_slider = SimbleeForMobile.drawSlider(95, 105, 155, 2, 50);
  spd_textfield = SimbleeForMobile.drawTextField(255, 105, 40, spd, "", TEXT_COLOR, bkgnd_color);
  updateSpeed();
  
  // Forecolor slider
  SimbleeForMobile.drawText(15, 150, "Forecolor", TEXT_COLOR);
  drawColorBar(95, 145, 155, 30);
  forecolor_slider = SimbleeForMobile.drawSlider(95, 145, 155, 0, 255);
  forecolor_swatch = SimbleeForMobile.drawRect(255, 145, 40, 30, getColorFromHue(forecolor));
  updateForeColor();
  
  // Backcolor slider
  SimbleeForMobile.drawText(15, 190, "Backcolor", TEXT_COLOR);
  drawColorBar(95, 185, 155, 30);
  backcolor_slider = SimbleeForMobile.drawSlider(95, 185, 155, 0, 255);
  backcolor_swatch = SimbleeForMobile.drawRect(255, 185, 40, 30, getColorFromHue(backcolor));
  updateBackColor();
  
  // Show
  SimbleeForMobile.drawText(30, 235, "Show", TEXT_COLOR);
  show_stepper = SimbleeForMobile.drawStepper(95, 232, 100, -1, MAX_SHOW);
  show_textfield = SimbleeForMobile.drawTextField(255, 235, 40, current_show, "", TEXT_COLOR, bkgnd_color);
  updateShows();

  // 3 Toggle switches for on, noise, palette
  int switch_height = 400;
  int switch_text_height = switch_height + 5;
  
  SimbleeForMobile.drawText(10, switch_text_height, "On", TEXT_COLOR);
  on_switch = SimbleeForMobile.drawSwitch(35, switch_height);
  SimbleeForMobile.updateValue(on_switch, 1);

  SimbleeForMobile.drawText(95, switch_text_height, "Noise", TEXT_COLOR);
  noise_switch = SimbleeForMobile.drawSwitch(137, switch_height, RED);
  SimbleeForMobile.updateValue(noise_switch, 0);

  SimbleeForMobile.drawText(201, switch_text_height, "Palette", TEXT_COLOR);
  palette_switch = SimbleeForMobile.drawSwitch(255, switch_height, BLUE);
  SimbleeForMobile.updateValue(palette_switch, 0);

  // Palette center slider
  palette_center_text = SimbleeForMobile.drawText(10, 450, "Center", TEXT_COLOR);
  drawPaletteCenterRects(70, 447, 175, 30);
  palette_center_slider = SimbleeForMobile.drawSlider(70, 445, 180, 0, 255);
  palette_center_swatch = SimbleeForMobile.drawRect(255, 445, 40, 30, getColorFromHue(forecolor));
  updatePaletteCenter();
  
  // Palette width slider
  palette_width_text = SimbleeForMobile.drawText(10, 490, "Width", TEXT_COLOR);
  palette_width_slider = SimbleeForMobile.drawSlider(70, 485, 180, 0, 255, palette_width);
  palette_width_textfield = SimbleeForMobile.drawTextField(255, 482, 40, palette_width, "", TEXT_COLOR, bkgnd_color);
  updatePaletteWidth();
  
  // Palette swatch
  uint8_t width = 10;//FULL_WIDTH / (palette_swatch_number - 1);
  uint8_t swatch_x = 50;
  for (uint8_t i=0; i < palette_swatch_number; i++) {
    palette_swatch[i] = SimbleeForMobile.drawRect(swatch_x, 525, width, 30, getColorForPaletteSwatch(i));
    swatch_x += width;
  }

  turnOnPaletteDetails();
  
  SimbleeForMobile.endScreen();
  
  // update(); // is this necessary?
}

//
//  Communicate changes back to board
//
void update() {

  // Brightness slider
  if (eventId == brightness_slider || eventId == brightness_textfield) {
    updateBrightness();
  
  // Speed Slider
  } else if (eventId == spd_slider || eventId == spd_textfield) {  
    updateSpeed();
  
  // Forecolor slider
  } else if (eventId == forecolor_slider) {
    updateForeColor();
  
  // Backcolor slider
  } else if (eventId == backcolor_slider) {
    updateBackColor();
  
  // Shows
  } else if (eventId == show_stepper || eventId == show_textfield) {
    updateShows();
  
  // Switches
  } else if (eventId == on_switch) {
    return;
  } else if (eventId == noise_switch) {
    return;
  } else if (eventId == palette_switch) {
    if (!do_palette) {  // counter-intuitive !
      turnOnPaletteDetails();
    } else {
      turnOffPaletteDetails();
    }
    return;

  // Palette center slider
  } else if (eventId == palette_center_slider) {
    updatePaletteCenter();
    updatePaletteSwatch();
  
  // Palette width slider
  } else if (eventId == palette_width_slider) {
    updatePaletteWidth();
    updatePaletteSwatch();
  }
}

void updateBrightness() {
  SimbleeForMobile.updateValue(brightness_slider, brightness);
  SimbleeForMobile.updateValue(brightness_textfield, brightness);
}

void updateSpeed() {
  SimbleeForMobile.updateValue(spd_slider, spd);
  SimbleeForMobile.updateValue(spd_textfield, spd);
}

void updateForeColor() {
  SimbleeForMobile.updateValue(forecolor_slider, forecolor);
  SimbleeForMobile.updateColor(forecolor_swatch, getColorFromHue(forecolor));
}

void updateBackColor() {
  SimbleeForMobile.updateValue(backcolor_slider, backcolor);
  SimbleeForMobile.updateColor(backcolor_swatch, getColorFromHue(backcolor)); 
}

void updateShows() {
  if (current_show == MAX_SHOW) {
    current_show -= MAX_SHOW;
  } else if (current_show == -1) {
    current_show = MAX_SHOW - 1;
  }
  SimbleeForMobile.updateValue(show_stepper, current_show);
  SimbleeForMobile.updateValue(show_textfield, current_show);
}

void updatePaletteCenter() {
  SimbleeForMobile.updateValue(palette_center_slider, palette_center);
  SimbleeForMobile.updateColor(palette_center_swatch, getColorFromHue(palette_center));  
}

void updatePaletteWidth() {
  SimbleeForMobile.updateValue(palette_width_slider, palette_width);
  SimbleeForMobile.updateValue(palette_width_textfield, palette_width);
}

//
//  ui_event - define callbacks
//
void ui_event(event_t &event) {
  eventId = event.id;
  
  if (event.id == brightness_slider || event.id == brightness_textfield) {
    brightness = event.value;
  } else if (event.id == spd_slider || event.id == spd_textfield) {
    spd = event.value;
  } else if (event.id == forecolor_slider) {
    forecolor = event.value;
    Serial.println(forecolor);  // Doubled!
  } else if (event.id == backcolor_slider) {
    backcolor = event.value;
  } else if (event.id == show_stepper) {
    current_show = event.value;
  } else if (event.id == on_switch) {
    is_on = event.value;
  } else if (event.id == noise_switch) {
    has_noise = event.value;
  } else if (event.id == palette_switch) {
    do_palette = event.value;
  } else if (event.id == palette_center_slider) {
    palette_center = event.value;
  } else if (event.id == palette_width_slider || event.id == palette_width_textfield) {
    palette_width = event.value;
  }
  needsUpdate = true;
}

color_t getColorFromHue(uint8_t hue) {
  CRGB color = CHSV(hue, 255, 255);  // FastLED does the CHSV -> CRGB for us!
  return rgb(color.r, color.g, color.b);  // Use rgb for Simblee
}

color_t getColorForPaletteSwatch(uint8_t i) {
  uint8_t hue = ((palette_center - (palette_width / 2)) + (i * (palette_width / palette_swatch_number))) % 256;
  return getColorFromHue(hue);
}

void updatePaletteSwatch() {
  for (uint8_t i=0; i < palette_swatch_number; i++) {
    SimbleeForMobile.updateColor(palette_swatch[i], getColorForPaletteSwatch(i));
  }
}

void turnOnPaletteDetails() {
  turnPaletteDetails(false);
}

void turnOffPaletteDetails() {
  turnPaletteDetails(true);
}

void turnPaletteDetails(bool state) {
  SimbleeForMobile.setVisible(palette_center_text, state);
  SimbleeForMobile.setVisible(palette_center_slider, state);
  SimbleeForMobile.setVisible(palette_center_swatch, state);
  SimbleeForMobile.setVisible(palette_width_text, state);
  SimbleeForMobile.setVisible(palette_width_slider, state);
  SimbleeForMobile.setVisible(palette_width_textfield, state);

  for (uint8_t i=0; i < palette_swatch_number; i++) {
    SimbleeForMobile.setVisible(palette_swatch[i], state);
  }
  for (uint8_t i=0; i < NUM_BARS; i++) {
    SimbleeForMobile.setVisible(palette_center_rects[i], state);
  }
}

void drawColorBar(int x, int y, int width, int height) {
  uint8_t barwidth = width / NUM_BARS;
  for (uint8_t i=0; i < NUM_BARS; i++) {
    x += barwidth;
    SimbleeForMobile.drawRect(x, y, barwidth, height, getColorFromHue(i * 256 / NUM_BARS));
  }
}

void drawPaletteCenterRects(int x, int y, int width, int height) {
  uint8_t barwidth = width / NUM_BARS;
  for (uint8_t i=0; i < NUM_BARS; i++) {
    x += barwidth;
    palette_center_rects[i] = SimbleeForMobile.drawRect(x, y, barwidth, height, getColorFromHue(i * 256 / NUM_BARS));
  }
}
