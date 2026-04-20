#include <pebble.h>
#include "pebble-graph/pebble-graph.h"

static Window *s_main_window;


static Layer *s_canvas_layer;
static TextLayer *x_axis_label_layer;
static TextLayer *y_axis_label_layer;

static uint8_t s_depth_data[182];
static uint16_t s_depth_data16[96];
static int16_t s_num_points = 0;
static char label_unit[] = "ft.";

static Graph depth_graph;

static void  axis_label(){
  text_layer_set_text(x_axis_label_layer, "One Week");
  text_layer_set_text(y_axis_label_layer, label_unit);
}
static void canvas_update_proc(Layer *layer, GContext *ctx){
  
  int depth_min = 2550;
  int depth_max = 50;
  
  for (int i = 0; i < s_num_points; i++){
    if (s_depth_data16[i] < depth_min){
      depth_min = s_depth_data16[i];
    }
    if (s_depth_data16[i] > depth_max){
      depth_max = s_depth_data16[i];
    }   
  }
  
  //if the min data point is at least 0.5 ft., then reduce the min by 0.5ft to create a small gap below min point on graph.
  if (depth_min >= 50){
    depth_min = depth_min - 50;
  }
  //if the max data point is less than 40 ft., then increase the max by 0.5 ft to create a small gap above highest point on the graph. 
  if (depth_max < (4000 - 50)){
      depth_max = depth_max + 50;
  }
  
//   depth_graph.chart_type = LINE;
//   depth_graph.graph_title = "Water Depth";
//   depth_graph.num_values = s_num_points;
  for (int i = 0; i < s_num_points; i++){
    //depth_graph.values[i] = s_depth_data16[i];
    printf("GraphValue: %i", s_depth_data16[i]);
    
  }
  printf("s_num_points: %i", s_num_points);
    
  GColor CanvasStrokeColor = PBL_IF_COLOR_ELSE(GColorDarkGreen, GColorBlack);
  GColor CanvasBackgroundColor = PBL_IF_COLOR_ELSE(GColorCyan, GColorWhite);
  
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, CanvasBackgroundColor);
  graphics_context_set_stroke_color(ctx, CanvasStrokeColor);
  graphics_context_set_antialiased(ctx, 1);
  

  GRect layer_bounds = layer_get_bounds(layer);
  int width = layer_bounds.size.w;
  int height = layer_bounds.size.h;
  
  
  //Draw graph border. Fill with color if available
  PBL_IF_COLOR_ELSE(graphics_fill_rect(ctx, GRect(0, 0, width, height), 0, GCornerNone), graphics_draw_rect(ctx, GRect(0, 0, width, height)));
  //graphics_fill_rect(ctx, GRect(0, 0, width, height), 0, GCornerNone);
  
  
   
  //printf("Min + Max: %i %i", depth_min, depth_max);
  
  uint16_t depth_range = depth_max - depth_min;
  printf("depth_range %i", depth_range);
  printf("depth_min %i", depth_min);
  printf("depth_max %i", depth_max);

  
  
  
  for (int i = 0; i < (s_num_points - 1); i++){
    
    
    int x1 = (i * width) / s_num_points;
    int x2 = ((i + 1) * width) / s_num_points;
    uint32_t y1 = height - ((height * (((s_depth_data16[i] - depth_min) * 1000) / depth_range)) / 1000);
    uint32_t y2 = height - ((height * (((s_depth_data16[i + 1] - depth_min) * 1000) / depth_range)) / 1000);

    
    printf("x1: %i", x1);
    printf("y1: %i", y1);
    printf("x2: %i", x2);
    printf("y2: %i", y2);
    
    graphics_draw_line(ctx, GPoint(x1, y1), GPoint(x2, y2));
    
  
  }
  
  
  //Draw y and x axis tic marks
  int num_tics_y = 7;
  int num_tics_x = 7;
  int x_scale = width / num_tics_x;
  int y_scale = height / num_tics_y;
  int tic_length = 5;
  int tic_offset = 5;
  static char buffer[10];
  uint16_t tic_label_offset = (((tic_offset * 1000) / height) * depth_range) / 1000;
  printf("y_scale, x_scale: %i %i", y_scale, x_scale);
  
  //Y-Axis
  for (int i = 1; i < num_tics_y; i++){
      //y-axis
      graphics_draw_line(ctx, GPoint(0, (height - (y_scale * i))), GPoint(tic_length, (height - (y_scale * i))));
      uint32_t tic_value = (((depth_max * 1000) - (((depth_range * 1000) / num_tics_y) * i)) / 1000);
      //printf("tic_value: %lu", tic_value);
      
      uint32_t tic_label = tic_value / 100;
      uint32_t tic_label_rem = tic_value % 100;
    
      int label_offset = 6;
      snprintf(buffer, sizeof(buffer), "%lu.%lu", tic_label, tic_label_rem);
      graphics_draw_text(ctx, buffer, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(tic_length + 3, ((y_scale * i) - label_offset), 35, 10), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    }
  //X-Axis
  for (int i = 1; i <= num_tics_x; i++){
    graphics_draw_line(ctx, GPoint(((x_scale * i)), (height)), GPoint(((x_scale * i)), (height - tic_length)));
  }
}
  

// AppMessage callbacks
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read tuples for weather data
   Tuple *depth_tuple = dict_find(iterator, MESSAGE_KEY_CHART_DATA);

if (depth_tuple) {
    // depth_tuple->value->data is our array of 96 bytes
    uint8_t *depth_data = depth_tuple->value->data;
    uint16_t num_points = depth_tuple->length;
    //printf("number of points: %i", num_points);
    //Copy data from app message into local variables
    s_num_points = num_points;
    memcpy(s_depth_data, depth_data, s_num_points);
  
    //shrink num_points to better fit on display
    s_num_points = s_num_points / 2;
  
    int j = 0;
  //reconstruct data into 16 bit numbers
    for (int i = 0; i < num_points; i+=2){
      uint16_t highBit = s_depth_data[i] << 8;
      uint16_t lowBit = s_depth_data[i+1];
      s_depth_data16[j] = highBit + lowBit;
      //printf("num_points %i", num_points);
      //printf("reconstructedBits: %i", s_depth_data16[j]);
      j++;
      
    }
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received %d points", num_points); 

  }
  
  //Update canvas
  if(s_canvas_layer) {
      layer_mark_dirty(s_canvas_layer);
      //printf("canvas updated. Latest height: %i", s_depth_data[s_num_points]);
    }
}


static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  
  int width = window_bounds.size.w;
  int height = window_bounds.size.h;
  

  
  // Create the canvas layer and axis label layer
  GRect bounds = layer_get_bounds(window_layer);
  s_canvas_layer = layer_create(GRect(10, 10, (bounds.size.w - 20), (bounds.size.h - 20)));
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  //Setup x axis text
  x_axis_label_layer = text_layer_create(GRect(0, (height-14), width, 25));
  text_layer_set_background_color(x_axis_label_layer, GColorClear);
  text_layer_set_text_color(x_axis_label_layer, GColorBlack);
  text_layer_set_font(x_axis_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(x_axis_label_layer, GTextAlignmentCenter);
  
  //Setup y axis text
  y_axis_label_layer = text_layer_create(GRect(0, (height/2), 25, height));
  text_layer_set_background_color(y_axis_label_layer, GColorClear);
  text_layer_set_text_color(y_axis_label_layer, GColorBlack);
  text_layer_set_font(y_axis_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(y_axis_label_layer, GTextAlignmentLeft);
  axis_label();
  
  
  layer_add_child(window_layer, s_canvas_layer);
  layer_add_child(window_layer, text_layer_get_layer(x_axis_label_layer));
  layer_add_child(window_layer, text_layer_get_layer(y_axis_label_layer));

  
  
}

static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  pebble_graph_destroy(&depth_graph);
  
}

static void init() {
  GColor  WindowColor = PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite);
  s_main_window = window_create();
   window_set_background_color(s_main_window, WindowColor);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);


  // Register AppMessage callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage

  app_message_open(256, 64);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

//App icon source: <a href="https://www.flaticon.com/free-icons/river" title="river icons">River icons created by kerismaker - Flaticon</a>
