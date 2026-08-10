import sys
import re

exit_code=0
re_errorcount = re.compile("<errorcounts>\s*(.*?)\s*</errorcounts>", re.S)
data = open(sys.argv[1], "r").read()
m = re_errorcount.findall(data)
for a in m:
    if a != "":
        print("Errors in valgrind log!")
        print(a)
        exit_code=1

# No errors in the last valgrind log section is accepted
re_last_section = re.compile("</errorcounts>\s*(.*?)\s*</valgrindoutput>", re.S)
section = re_last_section.findall(data)
last_section=""
for last_section in section:
    continue

if last_section != "":
    re_finished_error = re.compile("<error>\s*(.*?)\s*</error>", re.S)
    errs = re_finished_error.findall(last_section)
    for err in errs:
        print("Error when finished in valgrind log!")
        print(err)
        exit_code=1

sys.exit(exit_code)
