#ifndef LIGHT_LINK_H
#define LIGHT_LINK_H
#include <stdint.h>

typedef struct {
    unsigned unit, lamp, calibrated, mode, error, sample_seen;
    uint16_t raw, dark, bright, low, high;
    uint32_t last_sample, since, sum;
    unsigned count, minimum, maximum, dark_noise;
    /* TX timing contains no received characters. RX consumes ADC samples only. */
    unsigned tx, segment;
    uint32_t tx_at;
    unsigned level, candidate, armed, symbol_count, dropping, word_done;
    uint32_t candidate_at, edge_at;
    char symbols[6], received[22];
    unsigned received_count, errors;
} LightLink;

enum { LINK_IDLE, LINK_DARK_SETTLE, LINK_DARK_SAMPLE,
       LINK_BRIGHT_SETTLE, LINK_BRIGHT_SAMPLE, LINK_FINAL_SETTLE, LINK_LAMP };
enum { LINK_OK, LINK_NO_ADC, LINK_LOW_CONTRAST, LINK_TIMING, LINK_STOPPED };
void link_init(LightLink *s, uint32_t now);
void link_calibrate(LightLink *s, uint32_t now);
void link_stop(LightLink *s, uint32_t now);
void link_lamp(LightLink *s, unsigned on, uint32_t now);
int link_unit(LightLink *s, unsigned unit, uint32_t now);
int link_send_sos(LightLink *s, uint32_t now);
void link_task(LightLink *s, uint32_t now, unsigned has_sample, uint16_t raw);
const char *link_error(unsigned error);
#endif
