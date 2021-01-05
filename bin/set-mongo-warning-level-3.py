import os
from os.path import join

### 
def force_warning_level_3(file):
    with open(file, "r") as in_file:
        buf = in_file.readlines()

    push = '#pragma warning(push, 3)\n'
    pop  = '#pragma warning(pop)\n'

    last_line_has_mongo = False
    with open(file, "w") as out_file:
        for line in buf:            
            if line.startswith(('#include <mongo', '#include "mongo')):
                if last_line_has_mongo: 
                    pass # line = line + pop
                else:
                    line = push + line

                last_line_has_mongo = True
            else:
                if last_line_has_mongo: 
                    line = pop + line

                last_line_has_mongo = False          

            out_file.write(line)


### main
root_dir = 'E:\\robo\\src\\robomongo\\'
# root_dir = 'E:\\robo\\src\\robomongo\\shell\\bson\\'    # for testing
print('Processing all source files in root & sub directories...')
print('Root Dir:', root_dir)
print('---------------------------------------------------------')
for path, subdirs, files in os.walk(root_dir):
    for name in files:
        if name.endswith(('.h', '.cpp')):
            file = os.path.join(path, name)
            print(file)
            force_warning_level_3(file)