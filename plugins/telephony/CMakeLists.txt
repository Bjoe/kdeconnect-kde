find_package(KF5 REQUIRED COMPONENTS Notifications)

set(kdeconnect_telephony_SRCS
    telephonyplugin.cpp
)

kdeconnect_add_plugin(kdeconnect_telephony JSON kdeconnect_telephony.json SOURCES ${kdeconnect_telephony_SRCS})

target_link_libraries(kdeconnect_telephony
    kdeconnectcore
    KF5::I18n
    KF5::Notifications
)
