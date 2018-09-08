// Copyright 2016-2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <stdlib.h>

#include "doomdef.h"
#include "doomtype.h"
#include "m_argv.h"
#include "d_event.h"
#include "g_game.h"
#include "d_main.h"
#include "m_cheat.h"
#include "gamepad.h"
#include "lprintf.h"

#include "psxcontroller.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <driver/adc.h>
#include "../odroid/odroid_input.h"

//The gamepad uses keyboard emulation, but for compilation, these variables need to be placed
//somewhere. THis is as good a place as any.
int usejoystick=0;
int joyleft, joyright, joyup, joydown;
static odroid_gamepad_state previousJoystickState;

//atomic, for communication between joy thread and main game thread
volatile int joyVal=0;

typedef struct {
	int ps2mask;
	int *key;
} JsKeyMap;

//Mappings from PS2 buttons to keys
static const JsKeyMap keymap[]={
	{0x10, &key_up},
	{0x40, &key_down},
	{0x80, &key_left},
	{0x20, &key_right},

	{0x4000, &key_use},				//start
	{0x2000, &key_fire},			//circle
	{0x2000, &key_menu_enter},		//circle
	{0x8000, &key_pause},			//square
	{0x1000, &key_weapontoggle},	//triangle

	{0x8, &key_escape},				//menu
	{0x1, &key_map},				//select

	{0x400, &key_strafeleft},		//L1
	{0x100, &key_speed},			//L2
	{0x800, &key_straferight},		//R1
	{0x200, &key_strafe},			//R2

	{0, NULL},
};

int cheatCurrentLevel = 1;

int JoystickRead()
{
    odroid_gamepad_state state;

    odroid_input_gamepad_read(&state);

    
/*    lprintf(LO_INFO, "Kepad: Menu: %d, Volume: %d, Select: %d, Start: %d, Up: %d, Down: %d, Left: %d, Right: %d, A: %d, B: %d\n",
		    state.values[ODROID_INPUT_MENU], state.values[ODROID_INPUT_VOLUME], state.values[ODROID_INPUT_SELECT], state.values[ODROID_INPUT_START], 
		    state.values[ODROID_INPUT_UP], state.values[ODROID_INPUT_DOWN], state.values[ODROID_INPUT_LEFT], state.values[ODROID_INPUT_RIGHT], 
		    state.values[ODROID_INPUT_A], state.values[ODROID_INPUT_B]);
*/
	int result = 0;

	if (!state.values[ODROID_INPUT_UP])
		result |= 0x10; //key_up

	if (!state.values[ODROID_INPUT_DOWN])
		result |= 0x40; //key_down

	if (!state.values[ODROID_INPUT_LEFT])
		result |= 0x80; //key_left

	if (!state.values[ODROID_INPUT_RIGHT])
		result |= 0x20; //key_right

	if (!state.values[ODROID_INPUT_A])
		result |= 0x2000; //key_fire, key_menu_enter

	if (!state.values[ODROID_INPUT_START])
		result |= 0x4000; //key_use

	if (!state.values[ODROID_INPUT_MENU])
		result |= 0x8; //key_escape

	if (!state.values[ODROID_INPUT_SELECT])
		result |= 0x1000; //key_weapontoggle

	if (!state.values[ODROID_INPUT_VOLUME])
		result |= 0x1; //key_map
	
	if (!state.values[ODROID_INPUT_B]){
		result |= 0x200; //key_strafe
		result |= 0x100; //key_run
	}



	if (state.values[ODROID_INPUT_MENU] && state.values[ODROID_INPUT_START] && !previousJoystickState.values[ODROID_INPUT_START]){
		result |= 0x8000; //key_pause
	}

	if(state.values[ODROID_INPUT_MENU] && state.values[ODROID_INPUT_UP] && !previousJoystickState.values[ODROID_INPUT_UP]){
		//menu+up pressed at the same time
		char *code = "iddqd";
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		result = 0;
	}
	if(state.values[ODROID_INPUT_MENU] && state.values[ODROID_INPUT_DOWN] && !previousJoystickState.values[ODROID_INPUT_DOWN]){
		//menu+down pressed at the same time
		char *code = "idkfa";
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		result = 0;
	}
	if(state.values[ODROID_INPUT_MENU] && state.values[ODROID_INPUT_LEFT] && !previousJoystickState.values[ODROID_INPUT_LEFT]){
		//menu+left pressed at the same time
		char *code = "idclip";
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		result = 0;
	}
	if(state.values[ODROID_INPUT_MENU] && state.values[ODROID_INPUT_RIGHT] && !previousJoystickState.values[ODROID_INPUT_RIGHT]){
		//menu+right pressed at the same time
		char *code = "idbeholdh";
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		result = 0;
	}
	if(state.values[ODROID_INPUT_VOLUME] && state.values[ODROID_INPUT_UP] && !previousJoystickState.values[ODROID_INPUT_UP]){
		//volume+up pressed at the same time
		char *code = "idbeholdl";
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		result = 0;
	}
	if(state.values[ODROID_INPUT_VOLUME] && state.values[ODROID_INPUT_DOWN] && !previousJoystickState.values[ODROID_INPUT_DOWN]){
		//volume+down pressed at the same time
		char *code = "idbeholds";
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		result = 0;
	}
	if(state.values[ODROID_INPUT_VOLUME] && state.values[ODROID_INPUT_LEFT] && !previousJoystickState.values[ODROID_INPUT_LEFT]){
		//volume+left pressed at the same time
		char *code = "idbeholdi";
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		result = 0;
	}
	if(state.values[ODROID_INPUT_VOLUME] && state.values[ODROID_INPUT_RIGHT] && !previousJoystickState.values[ODROID_INPUT_RIGHT]){
		//volume+right pressed at the same time
		char code[9];
		lprintf(LO_INFO, "cheatCurrentLevel = %02d\n", cheatCurrentLevel);

		sprintf(code, "idclev%02d", cheatCurrentLevel);
		lprintf(LO_INFO, "idclev code: %s\n", code);
		int i;
		for(i=0; i<strlen(code); i++){
			M_FindCheats(code[i]);
		}
		cheatCurrentLevel++;
		if(cheatCurrentLevel > 32){
			cheatCurrentLevel = 1; 
		}
		result = 0;
	}

	previousJoystickState = state;

	return result;
}

void gamepadPoll(void)
{
	static int oldPollJsVal=0xffff;
	int newJoyVal=joyVal;
	event_t ev;

	for (int i=0; keymap[i].key!=NULL; i++) {
		if ((oldPollJsVal^newJoyVal)&keymap[i].ps2mask) {
			ev.type=(newJoyVal&keymap[i].ps2mask)?ev_keyup:ev_keydown;
			ev.data1=*keymap[i].key;
			D_PostEvent(&ev);
		}
	}

	oldPollJsVal=newJoyVal;
}



void jsTask(void *arg) {
	int oldJoyVal=0xFFFF;
	printf("Joystick task starting.\n");
	while(1) {
		vTaskDelay(20/portTICK_PERIOD_MS);
		joyVal=JoystickRead();
//		if (joyVal!=oldJoyVal) printf("Joy: %x\n", joyVal^0xffff);
		oldJoyVal=joyVal;
	}
}

void gamepadInit(void)
{
	lprintf(LO_INFO, "gamepadInit: Initializing game pad.\n");
}

void JoystickInit()
{
	odroid_input_gamepad_init();

}

void jsInit() {

	//Starts the js task
	JoystickInit();
	xTaskCreatePinnedToCore(&jsTask, "js", 5000, NULL, 7, NULL, 0);
}
