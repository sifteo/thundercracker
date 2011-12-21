#include "Base.h"

Cube::Side InferDirection(Vec2 u) {
	if (u.x > 0) {
		return SIDE_RIGHT;
	} else if (u.x < 0) {
		return SIDE_LEFT;
	} else if (u.y < 0) {
		return SIDE_TOP;
	} else {
		return SIDE_BOTTOM;
	}
}

bool InSpriteMode(Cube* c){
  uint8_t byte;
  _SYS_vbuf_peekb(&c->vbuf.sys, offsetof(_SYSVideoRAM, mode), &byte);
  return byte == _SYS_VM_BG0_SPR_BG1;
}

void EnterSpriteMode(Cube *c) {
  // Clear BG1/SPR before switching modes
  _SYS_vbuf_fill(&c->vbuf.sys, _SYS_VA_BG1_BITMAP/2, 0, 16);
  _SYS_vbuf_fill(&c->vbuf.sys, _SYS_VA_BG1_TILES/2, 0, 32);
  _SYS_vbuf_fill(&c->vbuf.sys, _SYS_VA_SPR, 0, 8*5/2);
  // Switch modes
  _SYS_vbuf_pokeb(&c->vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);
}

void SetSpriteImage(Cube *c, int id, int tile) {
  uint16_t word = VideoBuffer::indexWord(tile);
  uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].tile)/2 +
                   sizeof(_SYSSpriteInfo)/2 * id ); 
  _SYS_vbuf_poke(&c->vbuf.sys, addr, word);
}

void HideSprite(Cube *c, int id) {
  ResizeSprite(c, id, 0, 0);
}

void ResizeSprite(Cube *c, int id, int px, int py) {
  uint8_t xb = -px;
  uint8_t yb = -py;
  uint16_t word = ((uint16_t)xb << 8) | yb;
  uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].mask_y)/2 +
                   sizeof(_SYSSpriteInfo)/2 * id ); 
  _SYS_vbuf_poke(&c->vbuf.sys, addr, word);
}

void MoveSprite(Cube *c, int id, int px, int py) {
  uint8_t xb = -px;
  uint8_t yb = -py;
  uint16_t word = ((uint16_t)xb << 8) | yb;
  uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].pos_y)/2 +
                   sizeof(_SYSSpriteInfo)/2 * id ); 
  _SYS_vbuf_poke(&c->vbuf.sys, addr, word);
}