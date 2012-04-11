import Image, ImageSequence
import glob
import logging
import os.path
import sys

parts_size = (48, 48)
sprite_size = (64, 64)

names = {
    'Gluv' : 1,
    'Suli' : 2,
    'Rike' : 3,
    'Boff' : 4,
    'Zorg' : 5,
    'Maro' : 6,
    'Amor' : 7,
    'Droo' : 8,
    'Erol' : 9,
    'Flur' : 10,
    'Veax' : 11,
    'Yawp' : 12,
}

def ProcessBg(image_path):
    if not os.path.exists(image_path):
        print 'ERROR: Missing %s' % image_path
        return
    
    image = Image.open(image_path)
    image.load()
    
    buddy = os.path.basename(image_path)[:4]
    buddy_index = names[buddy]
    dir = os.path.join(os.path.dirname(image_path), 'output')
    name = 'bg%d.png' % names[buddy]
    image.save(os.path.join(dir, name))
    
def ProcessFull(image_bg_path, image_parts_path):
    if not os.path.exists(image_bg_path):
        print 'ERROR: Missing %s' % image_bg_path
        return
    
    if not os.path.exists(image_parts_path):
        print 'ERROR: Missing %s' % image_parts_path
        return
    
    image_bg = Image.open(image_bg_path)
    image_parts = Image.open(image_parts_path)
    
    image_bg.paste(image_parts, (0, 0), image_parts)
    image_bg = image_bg.crop((8, 8, 136, 136))
    
    buddy = os.path.basename(image_bg_path)[:4]
    buddy_index = names[buddy]
    dir = os.path.join(os.path.dirname(image_bg_path), 'output')
    name = 'buddy_full_%d.png' % names[buddy]
    image_bg.save(os.path.join(dir, name))
    
def ProcessParts(image_path):
    if not os.path.exists(image_path):
        print 'ERROR: Missing %s' % image_path
        return
    
    image = Image.open(image_path)
    image.load()
    
    boxes_src = [(48, 8, 96, 48), (8, 48, 48, 96), (48, 96, 96, 136), (96, 48, 136, 96)]
    
    num_frames_src = 4
    
    final_bg1 = Image.new('RGBA', (parts_size[0], parts_size[1] * num_frames_src * 4))
    final_sprite = Image.new('RGBA', (sprite_size[0], sprite_size[1] * num_frames_src * 4))
    
    for i in range(num_frames_src):
        box_src = boxes_src[i]
        frame = Image.new('RGBA', (48, 48))
        if i == 0:
            frame.paste(image.crop(box_src), (0, 0))
        elif i == 1:
            frame.paste(image.crop(box_src), (0, 0))
        elif i == 2:
            frame.paste(image.crop(box_src), (0, 8))
        elif i == 3:
            frame.paste(image.crop(box_src), (8, 0))
        for j in range(4):
            y = (parts_size[1] * j * 4) + (parts_size[1] * i)
            final_bg1.paste(frame, (0, y))
            #y = (sprite_size[1] * j * 4) + (sprite_size[1] * i) + offset
            #offset = (64 - 48) / 2
            #box_sprite = (offset, y)
            #final_sprite.paste(frame, box_sprite)
            frame = frame.rotate(90)
    
    buddy = os.path.basename(image_path)[:4]
    buddy_index = names[buddy]
    dir = os.path.join(os.path.dirname(image_path), 'output')
    
    name = 'parts%d.png' % names[buddy]
    final_bg1.save(os.path.join(dir, name))
    
    #name = 'parts%d_sprite.png' % names[buddy]
    #final_sprite.save(os.path.join(dir, name))
    
def ProcessFolder(image_folder):
    dir = os.path.join(image_folder, 'output')
    if not os.path.exists(dir):
        os.makedirs(dir)
    
    for buddy in names.keys():
        bg = os.path.join(image_folder, '%s-Bg.png' % buddy)
        parts = os.path.join(image_folder, '%s-Parts.png' % buddy)
    
        ProcessFull(bg, parts)
        ProcessParts(parts)
        ProcessBg(bg)

if __name__ == '__main__':
    ProcessFolder('parts_source')
