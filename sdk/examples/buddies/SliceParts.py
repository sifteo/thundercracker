import Image
import glob
import os.path
import sys

sprite_size = (64, 64)
num_parts = 4
num_rotations = 4

def ProcessFolder(image_dir_input, image_dir_output):
    [ProcessImage(png, image_dir_output) for png in glob.glob(os.path.join(image_dir_input, '*.png'))]

def ProcessImage(image_path, image_dir_output):
    image = Image.open(image_path)
    image.load()
    
    sprites = [Image.new('RGBA', sprite_size) for i in range(num_parts)]
    sprites[0].paste(image.crop((40,  0,  88,  40)), ( 8,  8))
    sprites[1].paste(image.crop(( 0, 40,  40,  88)), (12,  8))
    sprites[2].paste(image.crop((40, 88,  88, 128)), ( 8, 16))
    sprites[3].paste(image.crop((88, 40, 128,  88)), (12,  8))
    
    final = Image.new('RGBA', (sprite_size[0], sprite_size[1] * num_parts * num_rotations))
    PasteRotations(final, sprites)
    final.putdata(FilterPixels(final.getdata()))
    final.save(image_dir_output + os.path.basename(image_path))

def PasteRotations(image_final, sprites):
    sprite_index = 0
    for i in range(num_rotations):
        for sprite in [sprite.rotate(i * -(360.0 / num_rotations)) for sprite in sprites]:
            image_final.paste(sprite, (0, sprite_size[1] * sprite_index))
            sprite_index += 1

def FilterPixels(pixels):
    pixels_new = []
    for pixel in pixels:
        if pixel == (72, 255, 170, 255):
            pixels_new.append((255, 255, 255, 0))
        elif pixel == (72, 255, 255, 255):
            pixels_new.append((36, 218, 255, 255))
        else:
            pixels_new.append(pixel)
    return pixels_new

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print 'ERROR - Usage: SliceParts.py input_folder output_folder'
    else:
        ProcessFolder(sys.argv[1], sys.argv[2])