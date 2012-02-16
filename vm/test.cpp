#include <sifteo.h>
using namespace Sifteo;

Cube c;

void siftmain()
{
#if 0    // For split testing...
    _SYS_enableCubes(1);
    _SYS_enableCubes(2);
    _SYS_enableCubes(3);
    _SYS_enableCubes(4);
    _SYS_enableCubes(5);
    _SYS_enableCubes(6);
    _SYS_enableCubes(7);
    _SYS_enableCubes(8);
    _SYS_enableCubes(9);
    _SYS_enableCubes(10);
    _SYS_enableCubes(11);
    _SYS_enableCubes(12);
    _SYS_enableCubes(13);
    _SYS_enableCubes(14);
    _SYS_enableCubes(15);
    _SYS_enableCubes(16);
    _SYS_enableCubes(17);
    _SYS_enableCubes(18);
    _SYS_enableCubes(19);
    _SYS_enableCubes(20);
    _SYS_enableCubes(21);
    _SYS_enableCubes(22);
    _SYS_enableCubes(23);
    _SYS_enableCubes(24);
    _SYS_enableCubes(25);
    _SYS_enableCubes(26);
    _SYS_enableCubes(27);
    _SYS_enableCubes(28);
    _SYS_enableCubes(29);
    _SYS_enableCubes(30);
    _SYS_enableCubes(31);
    _SYS_enableCubes(32);
    _SYS_enableCubes(33);
    _SYS_enableCubes(34);
    _SYS_enableCubes(35);
    _SYS_enableCubes(36);
    _SYS_enableCubes(37);
    _SYS_enableCubes(38);
    _SYS_enableCubes(39);
    _SYS_enableCubes(40);
    _SYS_enableCubes(41);
    _SYS_enableCubes(42);
    _SYS_enableCubes(43);
    _SYS_enableCubes(44);
    _SYS_enableCubes(45);
    _SYS_enableCubes(46);
    _SYS_enableCubes(47);
    _SYS_enableCubes(48);
    _SYS_enableCubes(49);
    _SYS_enableCubes(50);
    _SYS_enableCubes(51);
    _SYS_enableCubes(52);
    _SYS_enableCubes(53);
    _SYS_enableCubes(54);
    _SYS_enableCubes(55);
    _SYS_enableCubes(56);
    _SYS_enableCubes(57);
    _SYS_enableCubes(58);
    _SYS_enableCubes(59);
    _SYS_enableCubes(60);
    _SYS_enableCubes(61);
    _SYS_enableCubes(62);
    _SYS_enableCubes(63);
    _SYS_enableCubes(64);
    _SYS_enableCubes(65);
    _SYS_enableCubes(66);
    _SYS_enableCubes(67);
    _SYS_enableCubes(68);
    _SYS_enableCubes(69);
    _SYS_enableCubes(70);
    _SYS_enableCubes(71);
    _SYS_enableCubes(72);
    _SYS_enableCubes(73);
    _SYS_enableCubes(74);
    _SYS_enableCubes(75);
    _SYS_enableCubes(76);
    _SYS_enableCubes(77);
    _SYS_enableCubes(78);
    _SYS_enableCubes(79);
    _SYS_enableCubes(80);
    _SYS_enableCubes(81);
    _SYS_enableCubes(82);
    _SYS_enableCubes(83);
    _SYS_enableCubes(84);
    _SYS_enableCubes(85);
    _SYS_enableCubes(86);
    _SYS_enableCubes(87);
    _SYS_enableCubes(88);
    _SYS_enableCubes(89);
    _SYS_enableCubes(90);
    _SYS_enableCubes(91);
    _SYS_enableCubes(92);
    _SYS_enableCubes(93);
    _SYS_enableCubes(94);
    _SYS_enableCubes(95);
    _SYS_enableCubes(96);
    _SYS_enableCubes(97);
    _SYS_enableCubes(98);
    _SYS_enableCubes(99);
    _SYS_enableCubes(100);
#endif
    
#if 1
    c.enable(0);
    VidMode_BG0_ROM vid(c.vbuf);

    vid.init();
    vid.BG0_text(Vec2(1,1), "Hello World!");

    while (1)
        System::paint();
#endif
}
