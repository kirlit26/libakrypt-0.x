# -------------------------------------------------------------------------------------------------- #
# пробуем выработать свежую версию документации для утилиты aktool
# -------------------------------------------------------------------------------------------------- #
find_program( PANDOC pandoc )
find_program( SED sed )
if( PANDOC )
  execute_process( COMMAND pandoc -s -t man --ascii
               ${CMAKE_SOURCE_DIR}/aktool/Readme.md -o ${CMAKE_SOURCE_DIR}/aktool/aktool.1 )
  message("-- Manual file aktool.1 updated" )
endif()

# -------------------------------------------------------------------------------------------------- #
# генерация файла для сборки документации (только для UNIX)
# -------------------------------------------------------------------------------------------------- #
if( LIBAKRYPT_HTML_DOC )
  find_file( DOXYGEN_BIN doxygen )
  if( DOXYGEN_BIN )
  # doxygen найден и документация может быть сгенерирована
     configure_file( ${CMAKE_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile @ONLY )
     file( WRITE ${CMAKE_BINARY_DIR}/make-html-${FULL_VERSION}.sh "#/bin/bash\n" )
     file( APPEND ${CMAKE_BINARY_DIR}/make-html-${FULL_VERSION}.sh "doxygen Doxyfile\n" )

     find_file( QHELPGENERATOR_BIN qhelpgenerator )
     if( QHELPGENERATOR_BIN )
       file( APPEND ${CMAKE_BINARY_DIR}/make-html-${FULL_VERSION}.sh
          "cp doc/html/libakrypt.qch ${CMAKE_BINARY_DIR}/libakrypt-doc-${FULL_VERSION}.qch\n" )
       file( APPEND ${CMAKE_BINARY_DIR}/make-html-${FULL_VERSION}.sh
                                                                "rm doc/html/libakrypt.qch\n" )
     endif()
     file( APPEND ${CMAKE_BINARY_DIR}/make-html-${FULL_VERSION}.sh
                                  "tar -cjvf libakrypt-doc-${FULL_VERSION}.tar.bz2 doc/html\n")

     execute_process( COMMAND chmod +x ${CMAKE_BINARY_DIR}/make-html-${FULL_VERSION}.sh )
     add_custom_target( html ${CMAKE_BINARY_DIR}/make-html-${FULL_VERSION}.sh )
     message("-- Script for documentation in HTML format is done (now \"make html\" enabled)")

     if( PANDOC )
       execute_process( COMMAND pandoc -f markdown -t latex --top-level-division=chapter -o ${CMAKE_BINARY_DIR}/readme.tex ${CMAKE_SOURCE_DIR}/Readme.md )
       if( SED )
         execute_process( COMMAND sed -i s/chapter/chapter*/g ${CMAKE_BINARY_DIR}/readme.tex )
         execute_process( COMMAND sed -i s/section/section*/g ${CMAKE_BINARY_DIR}/readme.tex )
       endif()
       message("-- English documentation in latex format updated")
     endif()
  else()
    message("-- doxygen not found")
    exit()
  endif()
endif()

# -------------------------------------------------------------------------------------------------- #
if( LIBAKRYPT_PDF_DOC )
  find_file( DOXYGEN_BIN doxygen )
  if( DOXYGEN_BIN )

  # doxygen найден и документация может быть сгенерирована
    set( LIBAKRYPT_PDF_HEADER "refman_header.tex" )
    set( LIBAKRYPT_PDF_FOOTER "refman_footer.tex" )

    configure_file( ${CMAKE_SOURCE_DIR}/doc/refman_header.in
                                               ${CMAKE_BINARY_DIR}/refman_header.tex @ONLY )
    configure_file( ${CMAKE_SOURCE_DIR}/doc/refman_footer.in
                                               ${CMAKE_BINARY_DIR}/refman_footer.tex @ONLY )

   # получаем документацию в формате PDF
    configure_file( ${CMAKE_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile @ONLY )
    file( WRITE ${CMAKE_BINARY_DIR}/make-pdf-${FULL_VERSION}.sh "#/bin/bash\n" )
    file( APPEND ${CMAKE_BINARY_DIR}/make-pdf-${FULL_VERSION}.sh "doxygen Doxyfile\n" )
    file( APPEND ${CMAKE_BINARY_DIR}/make-pdf-${FULL_VERSION}.sh "cd doc/latex\n" )
    file( APPEND ${CMAKE_BINARY_DIR}/make-pdf-${FULL_VERSION}.sh "make\n" )
    file( APPEND ${CMAKE_BINARY_DIR}/make-pdf-${FULL_VERSION}.sh
             "cp refman.pdf ${CMAKE_BINARY_DIR}/libakrypt-doc-${FULL_VERSION}.pdf\n" )
    file( APPEND ${CMAKE_BINARY_DIR}/make-pdf-${FULL_VERSION}.sh "cd ../..\n" )

    execute_process( COMMAND chmod +x ${CMAKE_BINARY_DIR}/make-pdf-${FULL_VERSION}.sh )
    add_custom_target( pdf ${CMAKE_BINARY_DIR}/make-pdf-${FULL_VERSION}.sh )
    message("-- Script for documentation in PDF format is done (now \"make pdf\" enabled)")
  endif()
endif()
