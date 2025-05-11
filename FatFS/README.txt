Download FatFS from the original source and store here before trying to compile the project.
Link to FatFS: https://elm-chan.org/fsw/ff/

FILES that should be stored in source/ folder:

  00readme.txt   This file.
  00history.txt  Revision history.
  ff.c           FatFs module.
  ffconf.h       Configuration file of FatFs module.
  ff.h           Common include file for FatFs and application module.
  diskio.h       Common include file for FatFs and disk I/O module.
  diskio.c       An example of glue function to attach existing disk I/O module to FatFs.
  ffunicode.c    Optional Unicode utility functions.
  ffsystem.c     An example of optional O/S related functions.


  Low level disk I/O module (diskio.c) is here only as a template. The low-level diskio.c developed for the project is stored in the corresponding folders (../device_sd_* folders)

