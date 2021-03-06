project(PYPES)

set(PYPES_SRCS
  pypebatch.py
  pypepad.py
  pypeserver.py
  pypetest.py
  pypetestrunner.py
  pypewrapper.py
  pyperun.py
  )

set(PYPES_BASE_SRCS
  pype.py
  pypescript.py
  pypes.py
  )

foreach (SCRIPT_FILE ${PYPES_BASE_SRCS})
  configure_file(${PYPES_SOURCE_DIR}/${SCRIPT_FILE} ${PYPES_BINARY_DIR}/${SCRIPT_FILE} COPYONLY)
endforeach (SCRIPT_FILE)

if(NOT PYPES_INSTALL_BIN_DIR)
  set(PYPES_INSTALL_BIN_DIR bin)
endif(NOT PYPES_INSTALL_BIN_DIR)

if(NOT PYPES_MODULE_INSTALL_LIB_DIR)
  set(PYPES_MODULE_INSTALL_LIB_DIR lib/${VMTK_PYTHON_VERSION}/site-packages/vmtk)
endif(NOT PYPES_MODULE_INSTALL_LIB_DIR)

foreach(SCRIPT_FILE ${PYPES_SRCS})
  configure_file(${PYPES_SOURCE_DIR}/${SCRIPT_FILE} ${PYPES_BINARY_DIR}/${SCRIPT_FILE} COPYONLY)
endforeach(SCRIPT_FILE)

install(FILES ${PYPES_BASE_SRCS} ${PYPES_SRCS}
  DESTINATION ${PYPES_MODULE_INSTALL_LIB_DIR}
  COMPONENT PythonRuntimeLibraries)

if(NOT WIN32 AND NOT VMTK_MINIMAL_INSTALL)

set(STRIPPED_PYPES_SRCS)
foreach (SCRIPT_FILE ${PYPES_SRCS})
  string(REGEX REPLACE ".py" "" STRIPPED_SCRIPT_FILE ${SCRIPT_FILE})
  # Rename script 'pype' to 'vmtk-pype' to avoid conflict 
  # with /usr/bin/pype in package 'pype' in Debian.
  if("${STRIPPED_SCRIPT_FILE}" STREQUAL "pype")
    set(STRIPPED_SCRIPT_FILE "vmtk-pype")
  endif("${STRIPPED_SCRIPT_FILE}" STREQUAL "pype")
  configure_file(${PYPES_SOURCE_DIR}/${SCRIPT_FILE} ${PYPES_BINARY_DIR}/${STRIPPED_SCRIPT_FILE})
  set (STRIPPED_PYPES_SRCS ${STRIPPED_PYPES_SRCS} ${PYPES_BINARY_DIR}/${STRIPPED_SCRIPT_FILE})
endforeach (SCRIPT_FILE)

install(PROGRAMS ${STRIPPED_PYPES_SRCS}
  DESTINATION ${PYPES_INSTALL_BIN_DIR}
  COMPONENT RuntimeExecutables)

endif(NOT WIN32 AND NOT VMTK_MINIMAL_INSTALL)
