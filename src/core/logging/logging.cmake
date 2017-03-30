
SET( WALBERLA_LOGLEVEL "PROGRESS" CACHE STRING "Set log level at compile time. Possible options: WARNING, INFO, PROGRESS, DETAIL, TRACING" )
SET_PROPERTY( CACHE WALBERLA_LOGLEVEL PROPERTY STRINGS WARNING INFO PROGRESS DETAIL TRACING )

SET( WALBERLA_LOGLEVEL_WARNING 1 )

IF( ${WALBERLA_LOGLEVEL} MATCHES "INFO" )
   SET( WALBERLA_LOGLEVEL_INFO 1 )
ENDIF()

IF( ${WALBERLA_LOGLEVEL} MATCHES "PROGRESS" )
   SET( WALBERLA_LOGLEVEL_INFO 1 )
   SET( WALBERLA_LOGLEVEL_PROGRESS 1 )
ENDIF()

IF( ${WALBERLA_LOGLEVEL} MATCHES "DETAIL" )
   SET( WALBERLA_LOGLEVEL_INFO 1 )
   SET( WALBERLA_LOGLEVEL_PROGRESS 1 )
   SET( WALBERLA_LOGLEVEL_DETAIL 1 )
ENDIF()

IF( ${WALBERLA_LOGLEVEL} MATCHES "TRACING" )
   SET( WALBERLA_LOGLEVEL_INFO 1 )
   SET( WALBERLA_LOGLEVEL_PROGRESS 1 )
   SET( WALBERLA_LOGLEVEL_DETAIL 1 )
   SET( WALBERLA_LOGLEVEL_TRACING 1 )
ENDIF()

configure_file ( logging/CMakeDefs.in.h logging/CMakeDefs.h )
