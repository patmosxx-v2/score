set(SCORE_COTIRE True)
set(SCORE_COTIRE_DISABLE_UNITY True)
set(SCORE_SPLIT_DEBUG True)
set(SCORE_STATIC_PLUGINS True)

set(SCORE_AUDIO_PLUGINS True CACHE INTERNAL "")
if(NOT DEFINED DEPLOYMENT_BUILD)
  set(DEPLOYMENT_BUILD False)
endif()

include(all-plugins)
