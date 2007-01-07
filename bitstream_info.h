/*
 *  bitstream_info.h
 *  Perian
 *
 *  Created by Graham Booker on 1/6/07.
 *  Copyright 2007 Graham Booker. All rights reserved.
 *
 */

#include <AudioToolbox/AudioToolbox.h>
#include <QuickTime/QuickTime.h>

int parse_ac3_bitstream(AudioStreamBasicDescription *asbd, AudioChannelLayout *acl, uint8_t *buffer, int buff_size);
