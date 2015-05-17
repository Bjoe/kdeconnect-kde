
if(KDECONNECT_CORE_ONLY)
  set(backends_kdeconnect_SRCS
    ${backends_kdeconnect_SRCS}

    backends/lan/lanlinkprovider.cpp
    backends/lan/landevicelink.cpp
    backends/lan/uploadjob.cpp
    backends/lan/downloadjob.cpp
    backends/lan/socketlinereader.cpp

    PARENT_SCOPE
  )
else()
  set(backends_kdeconnect_SRCS
    ${backends_kdeconnect_SRCS}

    backends/lan/lanlinkprovider.cpp
    backends/lan/landevicelink.cpp
    backends/lan/uploadjob.cpp
    backends/lan/downloadjob.cpp
    backends/lan/socketlinereader.cpp

    PARENT_SCOPE
  )
endif()
