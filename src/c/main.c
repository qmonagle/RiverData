#include <pebble.h>
#include <math.h>


static Window *s_main_window;


static Layer *s_canvas_layer;
static Layer *s_tic_layer;
static TextLayer *x_axis_label_layer;
static TextLayer *graph_title_layer;

static uint8_t s_depth_data[192];
static uint16_t s_depth_data16[96];
static int16_t s_num_points = 0;
static char label_unit[10] = "";
static char graph_title[15] = "Depth";



int depth_min = 2550;
int depth_max = 50;

static void  axis_label(){
  text_layer_set_text(x_axis_label_layer, "One Week");
  snprintf(graph_title, sizeof(graph_title), "Depth %s", label_unit);
  text_layer_set_text(graph_title_layer, graph_title);
}

//tic mark update
static void tic_update_proc(Layer *layer, GContext *ctx){
  GColor CanvasStrokeColor = PBL_IF_COLOR_ELSE(GColorDarkGreen, GColorBlack);
  GColor CanvasBackgroundColor = PBL_IF_COLOR_ELSE(GColorCyan, GColorWhite);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, CanvasBackgroundColor);
  graphics_context_set_stroke_color(ctx, CanvasStrokeColor);
  graphics_context_set_antialiased(ctx, 1);
  GRect tic_bounds = layer_get_bounds(layer);
  GRect graph_bounds = layer_get_bounds(s_canvas_layer);
  
  int tic_width = tic_bounds.size.w;
  int tic_height = tic_bounds.size.h;
  int graph_width = graph_bounds.size.w;
  int graph_height = graph_bounds.size.h;
  
  uint16_t depth_range = depth_max - depth_min;
  //avoid depth range of 0
  if (depth_range == 0){
    depth_range = 1;
  }
  
  //Draw y and x axis tic marks
  int num_tics_y = 7;
  int num_tics_x = 7;
  int x_scale = graph_width / num_tics_x;
  int y_scale = graph_height / num_tics_y;

  //int tic_offset = 5;
  static char buffer[10];
  //uint16_t tic_label_offset = (((tic_offset * 1000) / height) * depth_range) / 1000;
  int label_offset = 6;
  printf("graph width + height: %i %i", graph_width, graph_height);
  printf("y_scale, x_scale: %i %i", y_scale, x_scale);
  
  //Y-Axis
  for (int i = 1; i < num_tics_y; i++){
      //y-axis. Draw lines relative to graph area. and screen. Remember this coordinate system is based on the tic_layer and not the screen.
      graphics_draw_line(ctx, GPoint(27, (graph_height - (y_scale * i))), GPoint((tic_width - 9), (graph_height - (y_scale * i))));
      uint32_t tic_value = (((depth_max * 1000) - (((depth_range * 1000) / num_tics_y) * i)) / 1000);
      //printf("tic_value: %lu", tic_value);
      
      uint32_t tic_label = tic_value / 100;
      uint32_t tic_label_rem = tic_value % 100;
      tic_label_rem = tic_label_rem / 10;    
      
      snprintf(buffer, sizeof(buffer), "%lu.%lu", tic_label, tic_label_rem);
      graphics_draw_text(ctx, buffer, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(0, ((y_scale * i) - label_offset), 35, 10), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    }
  //X-Axis
  for (int i = 1; i <= (num_tics_x - 1); i++){
    graphics_draw_line(ctx, GPoint(((x_scale * i) + 27), (graph_height)), GPoint(((x_scale * i) + 27), 0));
  }
  
  
}

//Graph Update. Must be called first
static void canvas_update_proc(Layer *layer, GContext *ctx){
  //Reset min and max to prevent multiple canvas updates adding/subtracting 50 multiple times.
  depth_min = 65535;
  depth_max = 50;
 
  
  for (int i = 0; i < s_num_points; i++){
    if (s_depth_data16[i] < depth_min){
      depth_min = s_depth_data16[i];
    }
    if (s_depth_data16[i] > depth_max){
      depth_max = s_depth_data16[i];
    }   
  }
  //printf("unit label: %s", label_unit);
  if (strcmp(label_unit, "(m)") == 0){
    //if the min data point is at least 0.15 m, then reduce the min by 0.15 m to create a small gap below min point on graph.
    if (depth_min >= 15){
      depth_min = depth_min - 15;
    }
    //if the max data point is less than 12.19, then increase the max by 0.15 m to create a small gap above highest point on the graph. 
    if (depth_max < (1219 - 15)){
        depth_max = depth_max + 15;
    }
    //printf("min_max: %i %i", depth_min, depth_max);
  }else{
    //if the min data point is at least 0.5 ft., then reduce the min by 0.5ft to create a small gap below min point on graph.
    if (depth_min >= 50){
      depth_min = depth_min - 50;
    }
    //if the max data point is less than 40 ft., then increase the max by 0.5 ft to create a small gap above highest point on the graph. 
    if (depth_max < (65535 - 50)){
        depth_max = depth_max + 50;
    }
    
  }

  
    
  GColor CanvasStrokeColor = PBL_IF_COLOR_ELSE(GColorDarkGreen, GColorBlack);
  GColor CanvasBackgroundColor = PBL_IF_COLOR_ELSE(GColorCyan, GColorWhite);
  
  graphics_context_set_stroke_width(ctx, 2);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, CanvasBackgroundColor);
  graphics_context_set_stroke_color(ctx, CanvasStrokeColor);
  graphics_context_set_antialiased(ctx, 1);
  

  GRect layer_bounds = layer_get_bounds(layer);
  int width = layer_bounds.size.w;
  int height = layer_bounds.size.h;
  
  
  //Draw graph border. Fill with color if available
  PBL_IF_COLOR_ELSE(graphics_fill_rect(ctx, GRect(0, 0, width, height), 0, GCornerNone), graphics_draw_rect(ctx, GRect(0, 0, width, height)));
  
  uint16_t depth_range = depth_max - depth_min;
  //check to avoid divide by 0 error
  if (depth_range == 0){
    depth_range = 1;
  }
  for (int i = 0; i < (s_num_points - 1); i++){
    
    
    int x1 = (i * width) / s_num_points;
    int x2 = ((i + 1) * width) / s_num_points;
    uint32_t y1 = height - ((height * (((s_depth_data16[i] - depth_min) * 1000) / depth_range)) / 1000);
    uint32_t y2 = height - ((height * (((s_depth_data16[i + 1] - depth_min) * 1000) / depth_range)) / 1000);

    
//     printf("x1: %i", x1);
//     printf("y1: %i", y1);
//     printf("x2: %i", x2);
//     printf("y2: %i", y2);
    
    graphics_draw_line(ctx, GPoint(x1, y1), GPoint(x2, y2));
    
  
  }
}
  

// AppMessage callbacks
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read tuples for weather data
   Tuple *depth_tuple = dict_find(iterator, MESSAGE_KEY_CHART_DATA);
   Tuple *depth_tuple_unit = dict_find(iterator, MESSAGE_KEY_UNITS);
  
  
if (depth_tuple_unit){
  uint8_t depth_unit = depth_tuple_unit->value->uint8;
  uint8_t unit = depth_unit;
  if (unit == 0){
    strcpy(label_unit, "(ft)");
  }else if (unit == 1){
    strcpy(label_unit, "(m)");
  }else{
    strcpy(label_unit, "(ft)");
  }
  
  
}

if (depth_tuple) {
    // depth_tuple->value->data is our array of 96 bytes
    uint8_t *depth_data = depth_tuple->value->data;
    uint16_t num_points = depth_tuple->length;
  
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
  if(graph_title_layer){
    axis_label();
    layer_mark_dirty(text_layer_get_layer(graph_title_layer));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "graph title updated"); 
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
  s_canvas_layer = layer_create(GRect(27, 20, (bounds.size.w - 35), (bounds.size.h - 30)));
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  
  s_tic_layer = layer_create(GRect(0, 20, (bounds.size.w), (bounds.size.h - 30)));
  layer_set_update_proc(s_tic_layer, tic_update_proc);
  
  GRect graph_bounds = layer_get_bounds(s_canvas_layer);
  int graph_width = graph_bounds.size.w;
  
  //Set up x axis text
  x_axis_label_layer = text_layer_create(GRect(27, (height-14), graph_width, 25));
  text_layer_set_background_color(x_axis_label_layer, GColorClear);
  text_layer_set_text_color(x_axis_label_layer, GColorBlack);
  text_layer_set_font(x_axis_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(x_axis_label_layer, GTextAlignmentCenter);
  
  //Set up Graph Title
  graph_title_layer = text_layer_create(GRect(27, 0, graph_width, 20));
  text_layer_set_background_color(graph_title_layer, GColorClear);
  text_layer_set_text_color(graph_title_layer, GColorBlack);
  text_layer_set_font(graph_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(graph_title_layer, GTextAlignmentCenter);
  axis_label();
  
  
  layer_add_child(window_layer, s_canvas_layer);
  layer_add_child(window_layer, s_tic_layer);
  layer_add_child(window_layer, text_layer_get_layer(x_axis_label_layer));
  layer_add_child(window_layer, text_layer_get_layer(graph_title_layer));

  
  
}

static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  layer_destroy(s_tic_layer);
  text_layer_destroy(x_axis_label_layer);
  text_layer_destroy(graph_title_layer);
  
  
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
