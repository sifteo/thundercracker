import Image
import glob
import os.path
import sys

parts_size = (48, 48)
sprite_size = (64, 64)

def ProcessFolder(image_dir_input, image_dir_output):
    [ProcessImage(png, image_dir_output) for png in glob.glob(os.path.join(image_dir_input, 'parts*.png'))]

def ProcessImage(image_path, image_dir_output):
    image = Image.open(image_path)
    image.load()
    
    num_frames = image.size[1] / parts_size[1];
    
    final = Image.new('RGBA', (sprite_size[0], sprite_size[1] * num_frames))
    
    for i in range(num_frames):
        frame = image.copy().crop((0, parts_size[1] * (i + 0),  parts_size[0], parts_size[1] * (i + 1)))
        
        offset = (64 - 48)/2
        box = (offset, sprite_size[1] * (i + 0) + offset,  sprite_size[0] - offset, sprite_size[1] * (i + 1) - offset)
        final.paste(frame, box)
    
    name, ext = os.path.splitext(os.path.basename(image_path))
    final.save(image_dir_output + name + '_sprite' + ext)

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print 'ERROR - Usage: PartsToSprites.py input_folder output_folder'
    else:
        ProcessFolder(sys.argv[1], sys.argv[2])
