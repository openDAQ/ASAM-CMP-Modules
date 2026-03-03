set(OPENDAQ_ALWAYS_FETCH_ASAMCMPLIB ON CACHE BOOL "" FORCE)
set(ASAM_CMP_LIB_BUILD_EXAMPLE OFF)
set(ASAM_CMP_LIB_ENABLE_TESTS OFF)

opendaq_dependency(
    NAME                AsamCmpLib
    GIT_REPOSITORY      https://github.com/openDAQ/ASAM-CMP-Library.git 
    GIT_REF             v1.0.2-rc
    EXPECT_TARGET       asam_cmp
)
