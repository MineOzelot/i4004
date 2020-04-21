#ifndef I4004_RAM_H
#define I4004_RAM_H

#include <stdint.h>

#define RAM_CHIP_BYTES 32
#define RAM_CHIP_STATUS_BYTES 8

typedef struct ram_chip ram_chip;

typedef struct {
	void (*write_half)(ram_chip *, uint8_t, uint8_t, uint8_t);
	uint8_t (*read_half)(ram_chip *, uint8_t, uint8_t);
	void (*on_read_half)(ram_chip *, uint8_t, uint8_t);

	void (*write_status)(ram_chip *, uint8_t, uint8_t, uint8_t);
	uint8_t (*read_status)(ram_chip *, uint8_t, uint8_t);
	void (*on_read_status)(ram_chip *, uint8_t, uint8_t);

	void (*write_out)(ram_chip *, uint8_t);
} ram_interface;

struct ram_chip {
	struct vm_state *vm;

	uint8_t data[RAM_CHIP_BYTES];
	uint8_t status[RAM_CHIP_STATUS_BYTES];

	ram_interface interface;
};

ram_chip *ram_create(struct vm_state *vm);

void ram_write_half(ram_chip *ram, uint8_t reg, uint8_t half, uint8_t data);
uint8_t ram_read_half(ram_chip *ram, uint8_t reg, uint8_t half);

void ram_write(ram_chip *ram, uint8_t reg, uint8_t half, uint64_t data, int size);
uint64_t ram_read(ram_chip *ram, uint8_t reg, uint8_t half, int size);

static inline void ram_write_byte(ram_chip *ram, uint8_t reg, uint8_t half, uint8_t data) {
	ram_write(ram, reg, half, data, 2);
}

static inline uint8_t ram_read_byte(ram_chip *ram, uint8_t reg, uint8_t half) {
	return (uint8_t) ram_read(ram, reg, half, 2);
}

static inline void ram_write_word(ram_chip *ram, uint8_t reg, uint8_t half, uint16_t data) {
	ram_write(ram, reg, half, data, 4);
}

static inline uint16_t ram_read_wold(ram_chip *ram, uint8_t reg, uint8_t half) {
	return (uint16_t) ram_read(ram, reg, half, 4);
}

static inline void ram_write_dword(ram_chip *ram, uint8_t reg, uint8_t half, uint32_t data) {
	ram_write(ram, reg, half, data, 8);
}

static inline uint32_t ram_read_dword(ram_chip *ram, uint8_t reg, uint8_t half) {
	return (uint32_t) ram_read(ram, reg, half, 8);
}

void ram_write_status(ram_chip *ram, uint8_t reg, uint8_t idx, uint8_t data);
uint8_t ram_read_status(ram_chip *ram, uint8_t reg, uint8_t idx);

static inline void ram_write_out(ram_chip *ram, uint8_t half) {
	if(ram->interface.write_out) ram->interface.write_out(ram, half);
}

void ram_destroy(ram_chip *ram);

#endif //I4004_RAM_H
