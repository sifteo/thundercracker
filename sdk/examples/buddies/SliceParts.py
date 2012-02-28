import Image
import glob
import os.path
import sys

sprite_size = (64, 64)

def ProcessFolder(image_dir_input, image_dir_output):
    [ProcessImage(png, image_dir_output) for png in glob.glob(os.path.join(image_dir_input, '*.png'))]

def ProcessImage(image_path, image_dir_output):
    image = Image.open(image_path)
    image.load()
    
    sprites = [Image.new('RGBA', sprite_size) for i in range(4)]
    sprites[0].paste(image.crop((40,  0,  88,  40)), ( 8,  8))
    sprites[1].paste(image.crop(( 0, 40,  40,  88)), (12,  8))
    sprites[2].paste(image.crop((40, 88,  88, 128)), ( 8, 16))
    sprites[3].paste(image.crop((88, 40, 128,  88)), (12,  8))
    
    final = Image.new('RGBA', (sprite_size[0], sprite_size[1] * len(sprites) * 4))
    
    index = 0
    for sprite in sprites:
        final.paste(sprite, (0, sprite_size[1] * index))
        index += 1
    
    sprites = [sprite.rotate(-90) for sprite in sprites]
    for sprite in sprites:
        final.paste(sprite, (0, sprite_size[1] * index))
        index += 1
    
    sprites = [sprite.rotate(-90) for sprite in sprites]
    for sprite in sprites:
        final.paste(sprite, (0, sprite_size[1] * index))
        index += 1
    
    sprites = [sprite.rotate(-90) for sprite in sprites]
    for sprite in sprites:
        final.paste(sprite, (0, sprite_size[1] * index))
        index += 1
    
    new_data = []
    for pixel in final.getdata():
        if pixel[0] == 72 and pixel[1] == 255 and pixel[2] == 170:
            new_data.append((255, 255, 255, 0))
        elif pixel[0] == 72 and pixel[1] == 255 and pixel[2] == 255:
            new_data.append((36, 218, 255))
        else:
            new_data.append(pixel)
    final.putdata(new_data)
    
    final.save(image_dir_output + os.path.basename(image_path))

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print 'Please pass in a folder name with the .pngs you want to slice.'
    else:
        ProcessFolder(sys.argv[1], sys.argv[2])