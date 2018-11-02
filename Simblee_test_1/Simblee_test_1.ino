//#include <FastLED.h>
#include <SimbleeForMobile.h>

//
//  Get Simblee to work with the most basic program
//
//  6/8/18
//
# define BKGND_COLOR  85,85,85  // r,g,b
# define TEXT_COLOR  WHITE
# define NORMAL_TEXT_SIZE  18
# define LRG_TEXT_SIZE     24

volatile bool needsUpdate;
uint8_t eventId;

// Widget identifiers (uint8_t)
uint8_t brightness_slider, brightness_textfield;
uint8_t spd_slider, spd_textfield;
uint8_t forecolor_slider, forecolor_swatch;
uint8_t backcolor_slider, backcolor_swatch;
uint8_t show_segment;
uint8_t on_switch, noise_switch, palette_switch;
uint8_t palette_center_slider, palette_center_swatch;
uint8_t palette_width_slider, palette_width_textfield;
uint8_t palette_swatch;  // in development

// Variables
uint8_t brightness = 255;
uint8_t spd = 10;
uint8_t forecolor = 0;
uint8_t backcolor = 128;
uint8_t show_number = 0;
bool is_on, has_noise, do_palette;
uint8_t palette_center = 0;
uint8_t palette_width = 255;

void setup() {
  Serial.begin(9600);
  Serial.println("Simble Test started");

  SimbleeForMobile.deviceName = "Lights";  // Big name
  SimbleeForMobile.advertisementData = "Fish";  // Small name
  SimbleeForMobile.begin();
}

void loop() {
  if(needsUpdate) {
    update();
    needsUpdate = false;
  }
  SimbleeForMobile.process();  // end loop with this Simble function
}

//
//  ui - define buttons and sliders
//
void ui() {
  color_t bkgnd_color = rgb(BKGND_COLOR);
  
  // Background
  SimbleeForMobile.beginScreen(bkgnd_color);

  // Top text
  SimbleeForMobile.drawText(90, 25, "LED Controller", TEXT_COLOR, LRG_TEXT_SIZE);

  // Brightness slider - 40 vertical pixels between sliders
  /// ToDo: how to set a starting default
  SimbleeForMobile.drawText(10, 70, "Brightness", TEXT_COLOR);  // left_start, vert_position, text, color
  brightness_slider = SimbleeForMobile.drawSlider(95, 65, 155, 0, 255); // left_start, vert_pos, width, min, max
  brightness_textfield = SimbleeForMobile.drawTextField(255, 65, 40, brightness, "", TEXT_COLOR, bkgnd_color);  // left_start, vert_pos, width, start_value

  // Speed slider
  SimbleeForMobile.drawText(30, 110, "Speed", TEXT_COLOR);
  spd_slider = SimbleeForMobile.drawSlider(95, 105, 155, 2, 50);
  spd_textfield = SimbleeForMobile.drawTextField(255, 105, 40, spd, "", TEXT_COLOR, bkgnd_color);

  // Forecolor slider
  /// ToDo: put a wide-rectangle image behind of the color wheel: color_wheel = SimbleeForMobile.drawImage
  SimbleeForMobile.drawText(15, 150, "Forecolor", TEXT_COLOR);
  forecolor_slider = SimbleeForMobile.drawSlider(95, 145, 155, 0, 255);
  forecolor_swatch = SimbleeForMobile.drawRect(255, 145, 40, 30, getColorFromHue(forecolor));

  // Backcolor slider
  /// ToDo: put a wide-rectangle image behind of the color wheel: color_wheel = SimbleeForMobile.drawImage
  SimbleeForMobile.drawText(15, 190, "Backcolor", TEXT_COLOR);
  backcolor_slider = SimbleeForMobile.drawSlider(95, 185, 155, 0, 255);
  backcolor_swatch = SimbleeForMobile.drawRect(255, 185, 40, 30, getColorFromHue(backcolor));

  // Show
  SimbleeForMobile.drawText(140, 235, "Show", TEXT_COLOR);
  char *numbers[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };
  int segmentWidth = 295;
  /// ToDo: Lood up SimbleeForMobile.drawSegment
  show_segment = SimbleeForMobile.drawSegment(10, 265, segmentWidth, numbers, countof(numbers), TEXT_COLOR);
  SimbleeForMobile.updateValue(show_segment, show_number);

  // 3 Toggle switches for on, noise, palette
  int switch_height = 400;
  int switch_text_height = switch_height + 5;
  
  SimbleeForMobile.drawText(10, switch_text_height, "On", TEXT_COLOR);
  on_switch = SimbleeForMobile.drawSwitch(35, switch_height);
  SimbleeForMobile.updateValue(on_switch, 1);

  SimbleeForMobile.drawText(95, switch_text_height, "Noise", TEXT_COLOR);
  noise_switch = SimbleeForMobile.drawSwitch(137, switch_height);
  SimbleeForMobile.updateValue(noise_switch, 0);

  SimbleeForMobile.drawText(201, switch_text_height, "Palette", TEXT_COLOR);
  palette_switch = SimbleeForMobile.drawSwitch(255, switch_height);
  SimbleeForMobile.updateValue(palette_switch, 0);

  // Palette center slider
  /// ToDo: put a wide-rectangle image behind of the color wheel: color_wheel = SimbleeForMobile.drawImage
  SimbleeForMobile.drawText(10, 450, "Center", TEXT_COLOR);
  palette_center_slider = SimbleeForMobile.drawSlider(70, 445, 180, 0, 255, palette_center);
  palette_center_swatch = SimbleeForMobile.drawRect(255, 445, 40, 30, getColorFromHue(forecolor));

  // Palette width slider
  SimbleeForMobile.drawText(10, 490, "Width", TEXT_COLOR);
  palette_width_slider = SimbleeForMobile.drawSlider(70, 485, 180, 0, 255, palette_width);
  palette_width_textfield = SimbleeForMobile.drawTextField(255, 485, 40, palette_width, "", TEXT_COLOR, bkgnd_color);

  // Palette swatch
  palette_center_swatch = SimbleeForMobile.drawRect(10, 525, 295, 30, getColorFromHue(forecolor));
  
  SimbleeForMobile.endScreen();
  
  update();
}

//
//  Communicate changes back to board
//
void update() {

  // Brightness slider
  if (eventId == brightness_slider || eventId == brightness_textfield) {
    SimbleeForMobile.updateValue(brightness_slider, brightness);
    SimbleeForMobile.updateValue(brightness_textfield, brightness);
  
  // Speed Slider
  } else if (eventId == spd_slider || eventId == spd_textfield) {  
    SimbleeForMobile.updateValue(spd_slider, spd);
    SimbleeForMobile.updateValue(spd_textfield, spd);
  
  // Forecolor slider
  } else if (eventId == forecolor_slider) {
    SimbleeForMobile.updateValue(forecolor_slider, forecolor);
    SimbleeForMobile.updateColor(forecolor_swatch, getColorFromHue(forecolor));
  
  // Backcolor slider
  } else if (eventId == backcolor_slider) {
    SimbleeForMobile.updateValue(backcolor_slider, backcolor);
    SimbleeForMobile.updateColor(backcolor_swatch, getColorFromHue(backcolor));
  
  // Shows
  } else if (eventId == show_segment) {
    SimbleeForMobile.updateValue(show_segment, show_number);
  
  // Switches
  } else if (eventId == on_switch) {
    return;
  } else if (eventId == noise_switch) {
    return;
  } else if (eventId == palette_switch) {
    return;

  // Palette center slider
  } else if (eventId == palette_center_slider) {
    SimbleeForMobile.updateValue(palette_center_slider, palette_center);
    SimbleeForMobile.updateColor(palette_center_swatch, getColorFromHue(palette_center));
  
  // Palette width slider
  } else if (eventId == palette_width_slider) {
    SimbleeForMobile.updateValue(palette_width_slider, palette_width);
    SimbleeForMobile.updateValue(palette_width_textfield, palette_width);
  }
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
  } else if (event.id == backcolor_slider) {
    backcolor = event.value;
  } else if (event.id == show_segment) {
    show_number = event.value;
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
  // Need to convert hsv (hue, 255, 255) into rgb)
  return rgb(hue, 255, 0);  // dummy value
}

color_t calculatePalette() {
  // Lots to do
  return rgb(255, palette_center, palette_width);  // dummy value
}

void updatePaletteSwatch() {
  SimbleeForMobile.updateColor(palette_swatch, calculatePalette());
}
