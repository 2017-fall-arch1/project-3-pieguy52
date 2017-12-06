/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
 
#define GREEN_LED BIT6

char b1 = 0;
char b2 = 0;
char b3 = 0;
char b4 = 0;

char p1Score = 0;
char p2Score = 0;

AbRect paddle1 = {abRectGetBounds, abRectCheck, {10,1}}; //10 x 1 rectangle
AbRect paddle2 = {abRectGetBounds, abRectCheck, {10,1}};
AbRect net = {abRectGetBounds, abRectCheck, {62, 1}};

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 1, screenHeight/2 - 1}
};

Layer netLayer = {
  (AbShape *)&net,
  {(screenWidth/2), (screenHeight/2)},
  {0,0}, {0,0},
  COLOR_WHITE,
  0
};

Layer layer3 = {
  (AbShape *)&paddle2,
  {(screenWidth/2), (5)},
  {0,0}, {0,0},
  COLOR_WHITE,
  &netLayer
};

Layer layer2 = {
  (AbShape *)&paddle1,
  {(screenWidth/2), (screenHeight-5)},
  {0,0}, {0,0},
  COLOR_WHITE,
  &layer3
};
 
Layer fieldLayer = {		/* playing field as a layer */
(AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
 {0,0}, {0,0},				    /* last & next pos */
 COLOR_WHITE,
 &layer2
};


Layer layer0 = {
  (AbShape *)&circle4,
  {(screenWidth/2)+10, (screenHeight/2)+5},
  {0,0}, {0,0},
  COLOR_WHITE,
  &fieldLayer,
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
MovLayer ml3 = {&layer3, {3,0}, 0};
MovLayer ml2 = {&layer2, {3,0}, &ml3};
MovLayer ml0 = {&layer0, {2,2}, &ml2}; /**< not all layers move */

void buzzer(){
  for(int i = 0; i < 1000; i++){
    CCR0 = 1000;
    CCR1 = 1000 >> 5;
  }
  CCR0 = 0;
  CCR1 = 0;
}

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  



//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ball, MovLayer *paddle1, MovLayer *paddle2, Region *fence){
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary, p1Boundary, p2Boundary;
  char* score1[2];
  char* score2[2];
  itoa(p1Score, score1, 10);
  itoa(p2Score, score2, 10);
  if(b1 || b2 || b3 || b4){
    if(b1){
      vec2Add(&newPos, &paddle1->layer->posNext, &paddle1->velocity);
      abShapeGetBounds(paddle1->layer->abShape, &newPos, &shapeBoundary);
      if((shapeBoundary.topLeft.axes[0] < fence->topLeft.axes[0]) ||
	 (shapeBoundary.botRight.axes[0] > fence->botRight.axes[0])){
        int velocity = paddle1->velocity.axes[0] = -paddle1->velocity.axes[0];
	newPos.axes[0] += (2*velocity);
      }
      paddle1->layer->posNext = newPos;
    }else if(b2){
      paddle1->velocity.axes[0] = -paddle1->velocity.axes[0];
      vec2Add(&newPos, &paddle1->layer->posNext, &paddle1->velocity);
      abShapeGetBounds(paddle1->layer->abShape, &newPos, &shapeBoundary);
      if((shapeBoundary.topLeft.axes[0] < fence->topLeft.axes[0]) ||
	 (shapeBoundary.botRight.axes[0] > fence->botRight.axes[0])){
	int velocity = paddle1->velocity.axes[0] = -paddle1->velocity.axes[0];
	newPos.axes[0] += (2*velocity);
      }
      paddle1->layer->posNext = newPos;
    }else if(b3){
      vec2Add(&newPos, &paddle2->layer->posNext, &paddle2->velocity);
      abShapeGetBounds(paddle2->layer->abShape, &newPos, &shapeBoundary);
      if((shapeBoundary.topLeft.axes[0] < fence->topLeft.axes[0]) ||
	 (shapeBoundary.botRight.axes[0] > fence->botRight.axes[0])){
	int velocity = paddle2->velocity.axes[0] = -paddle2->velocity.axes[0];
	newPos.axes[0] += (2*velocity);
      }
      paddle2->layer->posNext = newPos;
    }else if(b4){
      paddle2->velocity.axes[0] = -paddle2->velocity.axes[0];
      vec2Add(&newPos, &paddle2->layer->posNext, &paddle2->velocity);
      abShapeGetBounds(paddle2->layer->abShape, &newPos, &shapeBoundary);
      if((shapeBoundary.topLeft.axes[0] < fence->topLeft.axes[0]) ||
	 (shapeBoundary.botRight.axes[0] > fence->botRight.axes[0])){
	int velocity = paddle2->velocity.axes[0] = -paddle2->velocity.axes[0];
	newPos.axes[0] += (2*velocity);
      }
      paddle2->layer->posNext = newPos;
    }
  }
  
  //moves ball
  vec2Add(&newPos, &ball->layer->posNext, &ball->velocity);
  abShapeGetBounds(ball->layer->abShape, &newPos, &shapeBoundary);
  //gets paddle1 pos
  abShapeGetBounds(paddle1->layer->abShape, &paddle1->layer->pos, &p1Boundary);
  //moves paddle2 pos
  abShapeGetBounds(paddle2->layer->abShape, &paddle2->layer->pos, &p2Boundary);

  //half way point of the ball
  int half = (shapeBoundary.topLeft.axes[0] + shapeBoundary.botRight.axes[0])/2;
  
  for (axis = 0; axis < 2; axis ++) {
    if(!axis){//axes is 1, Y axis
      if(half >= p1Boundary.topLeft.axes[0] &&
	 half <= p1Boundary.botRight.axes[0] &&
	 shapeBoundary.botRight.axes[1] > p1Boundary.topLeft.axes[1]){
	int velocity = ball->velocity.axes[1] = -ball->velocity.axes[1];
	newPos.axes[1] += (2*velocity);
	buzzer();
      }else if(half >= p2Boundary.topLeft.axes[0] &&
	       half <= p2Boundary.botRight.axes[0] &&
	       shapeBoundary.topLeft.axes[1] < p2Boundary.botRight.axes[1]){
	int velocity = ball->velocity.axes[1] = -ball->velocity.axes[1];
	newPos.axes[1] += (2*velocity);
	buzzer();
      }else if((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	 (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ||
	 (half >= p2Boundary.topLeft.axes[0] && half <= p2Boundary.botRight.axes[0]
	  && shapeBoundary.botRight.axes[1] > p1Boundary.topLeft.axes[1])
	 ){
      int velocity = ball->velocity.axes[axis] = -ball->velocity.axes[axis];
      newPos.axes[axis] += (2*velocity);
      buzzer();
      }
    }else{//axes is 0, X axis
      if(shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]){
	newPos.axes[0] = (screenWidth/2) + 10;
	newPos.axes[1] = (screenHeight/2) + 5;
	p1Score++;
      }else if(shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]){
	newPos.axes[0] = (screenWidth/2) + 10;
	newPos.axes[1] = (screenHeight/2) + 5;
	p2Score++;
      }
    }
  } /**< for axis */
  
  ball->layer->posNext = newPos;
  drawString5x7((screenWidth/2 - 12), (screenHeight/2 - 10), score1, COLOR_WHITE, COLOR_BLACK);
  drawString5x7((screenWidth/2 + 12), (screenHeight/2 - 10), score2, COLOR_WHITE, COLOR_BLACK);
}


u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */


/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  timerAUpmode(); //init of buzzer
  P2SEL2 &= ~(BIT6 | BIT7);
  P2SEL &= ~BIT7;
  P2SEL |= BIT6;
  P2DIR = BIT6;
  
  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();

  layerInit(&layer0);
  layerDraw(&layer0);

  layerGetBounds(&fieldLayer, &fieldFence);
  
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  char* str = "-";

  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &layer0);
    drawString5x7((screenWidth/2), (screenHeight/2 - 10), str, COLOR_WHITE, COLOR_BLACK); 
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  u_int switchVal = p2sw_read();
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count++;
  if (count == 15) {
     mlAdvance(&ml0, &ml2, &ml3, &fieldFence);
     if(~switchVal & 1){
      redrawScreen = 1;
      b2 = b3 = b4 = 0;
      b1 = 1;
     }else if(~switchVal & 2){
      redrawScreen = 1;
      b1 = b3 = b4 = 0;
      b2 = 1;
     }else if(~switchVal & 4){
      redrawScreen = 1;
      b1 = b2 = b4 = 0;
      b3 = 1;
     }else if(~switchVal & 8){
      redrawScreen = 1;
      b1 = b2 = b3 = 0;
      b4 = 1;
     }else{
       redrawScreen = 1;
      b1 = b2 = b3 = b4 = 0;
     }
    count = 0;
  } 
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
