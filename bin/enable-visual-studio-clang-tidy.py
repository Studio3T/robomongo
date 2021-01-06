import sys
from lxml import etree

print('\nEnabling Clang Tidy for Visual Studio 2019')

robo_proj_xml = sys.argv[1]
clang_tidy_xml = sys.argv[2] + '/clang-tidy.xml'
clang_checks_txt = sys.argv[2] + '/clang-checks.txt'
print(sys.argv[0] + ":\nProcessing: " + sys.argv[1])

## Read clang checks
clang_checks = None
with open(clang_checks_txt, "r") as file:
    clang_checks = file.read().replace('\n', ',')

## 
tree = etree.parse(robo_proj_xml)
ns = {'ns':'http://schemas.microsoft.com/developer/msbuild/2003'}
if(tree.find('//ns:EnableClangTidyCodeAnalysis', namespaces=ns) is not None):    
    tree.find('//ns:ClangTidyChecks', namespaces=ns).text = clang_checks
    tree.write(robo_proj_xml)    
    print('Finished')
    sys.exit()

## Append clang_tidy_xml  -  Todo: refactor using etree
with open(robo_proj_xml, "r") as in_file:
    buf = in_file.readlines()

with open(clang_tidy_xml, "r") as in_file:
    clang_tidy_xml_buf = in_file.read()

appended = False
with open(robo_proj_xml, "w") as out_file:
    for line in buf:
        if "</ItemGroup>" in line and appended == False:
            line = line + clang_tidy_xml_buf
            appended = True
        out_file.write(line)

## Add clang checks           
tree = etree.parse(robo_proj_xml)
tree.find('//ns:ClangTidyChecks', namespaces=ns).text = clang_checks
tree.write(robo_proj_xml)
print('Finished')