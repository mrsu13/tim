ifeq ($(TIM_PLATFORM),)
$(error ERROR: TIM_PLATFORM environment variable must be defined.)
endif

include build/$(TIM_PLATFORM)/platform.mk
