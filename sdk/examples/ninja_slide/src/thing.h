class Thing {
public:
    int id;
    Float2 pos;
    Float2 vel;
    Thing(int id, int x, int y);
    Thing(int id0, Int2 pos0);
    void update(VidMode_BG0_SPR_BG1 vid);
};

Thing::Thing(int id0, Int2 pos0){
    id = id0;
    pos = pos0;
}

void Thing::update(VidMode_BG0_SPR_BG1 vid){
    vid.moveSprite(id, pos.toInt());
}
