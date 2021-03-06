
include_directories(
${KOPETE_INCLUDES}
${KOPETE_BINARY_DIR}
)


########### next target ###############

set(kopete_history_PART_SRCS 
   historyplugin.cpp 
   historydialog.cpp 
   historylogger.cpp 
   converter.cpp 
   historyguiclient.cpp
   historyimport.cpp )

kconfig_add_kcfg_files(kopete_history_PART_SRCS historyconfig.kcfgc )

ki18n_wrap_ui(kopete_history_PART_SRCS historyviewer.ui)

add_library(kopete_history MODULE ${kopete_history_PART_SRCS})
set_target_properties(kopete_history PROPERTIES AUTOMOC TRUE)

target_link_libraries(kopete_history Qt5::Widgets KF5::KHtml kopete )

install(TARGETS kopete_history  DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### next target ###############

set(kcm_kopete_history_PART_SRCS historypreferences.cpp )

kconfig_add_kcfg_files(kcm_kopete_history_PART_SRCS historyconfig.kcfgc )

ki18n_wrap_ui(kcm_kopete_history_PART_SRCS historyprefsui.ui)

add_library(kcm_kopete_history MODULE ${kcm_kopete_history_PART_SRCS})
set_target_properties(kcm_kopete_history PROPERTIES AUTOMOC TRUE)


target_link_libraries(kcm_kopete_history KF5::KCMUtils Qt5::Widgets kopete )

install(TARGETS kcm_kopete_history  DESTINATION ${KDE_INSTALL_PLUGINDIR})


########### install files ###############

install( FILES historyconfig.kcfg  DESTINATION ${KDE_INSTALL_KCFGDIR})
install( FILES kopete_history.desktop  DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install( FILES historyui.rc  historychatui.rc  DESTINATION ${KDE_INSTALL_DATADIR}/kopete_history)
install( FILES kopete_history_config.desktop  DESTINATION ${KDE_INSTALL_KSERVICES5DIR}/kconfiguredialog)
