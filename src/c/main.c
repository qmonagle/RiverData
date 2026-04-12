#include <pebble.h>

static Window *s_main_window;


static Layer *s_canvas_layer;
static TextLayer *s_axis_label_layer;

static uint8_t s_depth_data[96];
static int16_t s_num_points = 0;


static void  axis_label(){
  text_layer_set_text(s_axis_label_layer, "One Week");
}

static void canvas_update_proc(Layer *layer, GContext *ctx){
 
  
  graphics_context_set_stroke_width(ctx, 2);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_context_set_antialiased(ctx, 1);
  
  GRect layer_bounds = layer_get_bounds(layer);
  int width = layer_bounds.size.w;
  int height = layer_bounds.size.h;
  int depth_min = 255;
  int depth_max = 50;
  
  for (int i = 0; i < s_num_points; i++){
    if (s_depth_data[i] < depth_min){
      depth_min = s_depth_data[i];
    }
    if (s_depth_data[i] > depth_max){
      depth_max = s_depth_data[i];
    }   
  }
   //if the min data point is at least 0.5 ft., then reduce the min by 0.5ft to create a small gap below min point on graph.
  if (depth_min >= 5){
    depth_min = depth_min - 5;
  }
  //if the max data point is less than 25 ft., then increase the max by 0.5 ft to create a small gap above highest point on the graph. 
  if (depth_max < (255 - 5)){
      depth_max = depth_max + 5;
  }
  //printf("Min + Max: %i %i", depth_min, depth_max);
  
  int depth_range = depth_max - depth_min;
  
  
  for (int i = 0; i < (s_num_points - 1); i++){
    
    
    int x1 = (i * width) / s_num_points;
    int x2 = ((i + 1) * width) / s_num_points;
    uint32_t y1 = height - ((height * (((s_depth_data[i] - depth_min) * 1000) / depth_range)) / 1000);
    uint32_t y2 = height - ((height * (((s_depth_data[i + 1] - depth_min) * 1000) / depth_range)) / 1000);

    
    //printf("x1: %i", x1);
    //printf("y1: %i", y1);
    //printf("x2: %i", x2);
    //printf("y2: %i", y2);
    
    graphics_draw_line(ctx, GPoint(x1, y1), GPoint(x2, y2));
    
  
  }
  
  
  //Draw y and x axis tic marks
  int num_tics_y = 7;
  int num_tics_x = 7;
  int x_scale = width / num_tics_x;
  int y_scale = height / num_tics_y;
  int tic_length = 5;
  //int tic_offset = 5;
  static char buffer[10];
  //uint16_t tic_label_offset = (((tic_offset * 1000) / height) * depth_range) / 1000;
  //printf("y_scale, x_scale: %i %i", y_scale, x_scale);
  
  //Y-Axis
  for (int i = 1; i < num_tics_y; i++){
      //y-axis
      graphics_draw_line(ctx, GPoint(0, (height - (y_scale * i))), GPoint(tic_length, (height - (y_scale * i))));
      uint32_t tic_value = (((depth_max * 1000) - (((depth_range * 1000) / num_tics_y) * i)) / 1000);
      //printf("tic_value: %lu", tic_value);
      
      uint32_t tic_label = tic_value / 10;
      uint32_t tic_label_rem = tic_value % 10;
    
      snprintf(buffer, sizeof(buffer), "%lu.%lu", tic_label, tic_label_rem);
      graphics_draw_text(ctx, buffer, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(tic_length + 3, ((y_scale * i) - 8), 35, 10), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    }
  //X-Axis
  for (int i = 1; i <= num_tics_x; i++){
    graphics_draw_line(ctx, GPoint(((x_scale * i)), (height)), GPoint(((x_scale * i)), (height - tic_length)));
  }
  
  
  //Draw graph border
  graphics_draw_rect(ctx,GRect(0, 0, width, height));
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
  
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received %d points", num_points); 

  }
  
  //Update canvas
  if(s_canvas_layer) {
      layer_mark_dirty(s_canvas_layer);
      printf("canvas updated. Latest height: %i", s_depth_data[s_num_points]);
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
  s_axis_label_layer = text_layer_create(GRect(0, (height-14), width, 25));
  text_layer_set_background_color(s_axis_label_layer, GColorClear);
  text_layer_set_text_color(s_axis_label_layer, GColorBlack);
  text_layer_set_font(s_axis_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_axis_label_layer, GTextAlignmentCenter);
  axis_label();
  
  
  layer_add_child(window_layer, s_canvas_layer);
  layer_add_child(window_layer, text_layer_get_layer(s_axis_label_layer));

  
  
}

static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorWhite);
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
