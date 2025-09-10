export CC  	 := ccache clang
export CPP 	 := ccache clang++
export AR    := ar cr
export STRIP := strip

include build/$(TIM_PLATFORM)/rules.mk
