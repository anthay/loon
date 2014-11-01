
# Publishing loon-cpp




## Release checklist

- unit test
- Consistent interfaces, parameter names
- Review comments
- Review TBDs
- Check spelling
- Line endings for all files LF (MSVC2010 does not mind *.vcxproj being LF)
- MSVC /W4 or /Wall(?)
- clang++ -Wall --analyze
- Comeau? lint?
- README.md
- LICENSE
- makefile


## Publish checklist

- Start with all code on Mac
- make test
- make clean
- README.mp
  - MUST REFLECT test.cpp REALITY
  - Update release note
- check everything in to git with "loon-cpp-1-00 release" in the message
- create release zip
  - Normalize all the line endings, if necessary
     - https://help.github.com/articles/dealing-with-line-endings/
  - copy loon dev dir to loon-cpp-1-00 (or whatever current release)
  - cd loon-cpp-1-00
  - rm loon-cpp-checklist.md
  - rm .git
  - rm .DS_Store in all directories (?)
  - compress loon-cpp-1-00 (use tar/zip?)
  - (problem unzipping on Windows - get encryped files (green))
- copy loon-cpp-1-00.zip to PC, unzip and test
