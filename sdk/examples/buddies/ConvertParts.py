import Image
import glob
import os.path
import sys

parts_size = (48, 48)
sprite_size = (64, 64)

def ProcessImage(image_path):
    image = Image.open(image_path)
    image.load()
    
    num_frames_src = 4
    
    final_bg1 = Image.new('RGBA', (parts_size[0], parts_size[1] * num_frames_src * 4))
    final_sprite = Image.new('RGBA', (sprite_size[0], sprite_size[1] * num_frames_src * 4))
    
    for i in range(num_frames_src):
        box_src = (0, parts_size[1] * (i + 0), parts_size[0], parts_size[1] * (i + 1))
        frame = image.copy().crop(box_src)
        for j in range(4):
            
            y = (parts_size[1] * j * 4) + (parts_size[1] * i)
            box_bg1 = (0, y, parts_size[0], y + parts_size[1])
            final_bg1.paste(frame, box_bg1)
            
            offset = (64 - 48) / 2
            y = (sprite_size[1] * j * 4) + (sprite_size[1] * i) + offset
            box_sprite = (offset, y,  sprite_size[0] - offset, y + parts_size[1])
            final_sprite.paste(frame, box_sprite)
            
            frame = frame.rotate(90)
    
    name, ext = os.path.splitext(image_path)
    name = name.replace('_source', '')
    
    final_bg1.save(name + ext)
    final_sprite.save(name + '_sprite' + ext)

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print 'ERROR - Usage: %s [INPUT]_source.png' % sys.argv[0]
    else:
        ProcessImage(sys.argv[1])
