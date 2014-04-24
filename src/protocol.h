#ifndef PROTOCOL_H
#define PROTOCOL_H

#define CMD_LENGTH    3

#define CMD_TEST_PACKETS    (unsigned char)0x31
#define CMD_TEST            (unsigned char)0x32
#define CMD_REAL_PACKETS    (unsigned char)0x33
#define CMD_REAL            (unsigned char)0x34
#define CMD_ABORT           (unsigned char)0x35

#define PREAMB_LENGTH       1
#define MESSAGE_LENGTH      (341 * 3 + PREAMB_LENGTH)
#define AUDIO_LENGTH        (MESSAGE_LENGTH - PREAMB_LENGTH)

#endif // PROTOCOL_H
