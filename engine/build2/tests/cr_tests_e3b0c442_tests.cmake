add_test([=[Fixup.ValidRecordApplies]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Fixup.ValidRecordApplies]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Fixup.ValidRecordApplies]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fixup.cpp:42]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Fixup.TornRecordRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Fixup.TornRecordRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Fixup.TornRecordRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fixup.cpp:53]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Fixup.TornThirdTailStillSalvaged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Fixup.TornThirdTailStillSalvaged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Fixup.TornThirdTailStillSalvaged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fixup.cpp:67]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Fixup.MinSizeGate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Fixup.MinSizeGate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Fixup.MinSizeGate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fixup.cpp:86]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Fixup.ZeroOffsetRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Fixup.ZeroOffsetRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Fixup.ZeroOffsetRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fixup.cpp:93]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Fixup.FourKnSectorSize]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Fixup.FourKnSectorSize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Fixup.FourKnSectorSize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fixup.cpp:102]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.FlatFileScansMbr]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.FlatFileScansMbr]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.FlatFileScansMbr]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:121]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.FlatRejectsEmptyAndMissing]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.FlatRejectsEmptyAndMissing]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.FlatRejectsEmptyAndMissing]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:153]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.TruncatedFlatImageCeilsSectorCount]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.TruncatedFlatImageCeilsSectorCount]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.TruncatedFlatImageCeilsSectorCount]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:163]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.FlatGeometryValidOnOddSizedImage]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.FlatGeometryValidOnOddSizedImage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.FlatGeometryValidOnOddSizedImage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:200]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.ImageRejectsTruncatedFile]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.ImageRejectsTruncatedFile]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.ImageRejectsTruncatedFile]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:215]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.EdiskRefusedAsRegisteredExclusion]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.EdiskRefusedAsRegisteredExclusion]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.EdiskRefusedAsRegisteredExclusion]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:229]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.BadMagicVmdkRefusedNotFlatFallback]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.BadMagicVmdkRefusedNotFlatFallback]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.BadMagicVmdkRefusedNotFlatFallback]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:237]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VmdkSparseExtentReads]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.VmdkSparseExtentReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VmdkSparseExtentReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:327]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VmdkDifferencingChainReadsParent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.VmdkDifferencingChainReadsParent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VmdkDifferencingChainReadsParent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:362]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VmdkStreamOptimizedRefused]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.VmdkStreamOptimizedRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VmdkStreamOptimizedRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:389]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VhdDynamicDisk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.VhdDynamicDisk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VhdDynamicDisk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:502]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VhdDifferencingReadsParentOnClearBitmap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.VhdDifferencingReadsParentOnClearBitmap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VhdDifferencingReadsParentOnClearBitmap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:530]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VhdBadChecksumRefused]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.VhdBadChecksumRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VhdBadChecksumRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:554]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VhdxDynamicReads]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.VhdxDynamicReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VhdxDynamicReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:711]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VhdxLogOverlayServesUnflushedWrites]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.VhdxLogOverlayServesUnflushedWrites]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VhdxLogOverlayServesUnflushedWrites]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:734]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VhdxDifferencingRefusedPrecisely]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.VhdxDifferencingRefusedPrecisely]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VhdxDifferencingRefusedPrecisely]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:785]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.Lba0LayoutClassification]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.Lba0LayoutClassification]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.Lba0LayoutClassification]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:800]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.ExtensionClassificationMatchesOgDispatch]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.ExtensionClassificationMatchesOgDispatch]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.ExtensionClassificationMatchesOgDispatch]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:811]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.SessionRoundTripPersistsImagePath]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.SessionRoundTripPersistsImagePath]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.SessionRoundTripPersistsImagePath]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:821]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.QemuVmdkCompat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.QemuVmdkCompat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.QemuVmdkCompat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:942]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.QemuVhdCompat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.QemuVhdCompat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.QemuVhdCompat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:967]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.QemuVhdxCompat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Image.QemuVhdxCompat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.QemuVhdxCompat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:991]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ImageCreate.HashMatchesSource]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ImageCreate.HashMatchesSource]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ImageCreate.HashMatchesSource]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:1082]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ImageCreate.ShortTailNotPadded]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ImageCreate.ShortTailNotPadded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ImageCreate.ShortTailNotPadded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:1097]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ImageCreate.FourKnGeometryStreamsUnpadded]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ImageCreate.FourKnGeometryStreamsUnpadded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ImageCreate.FourKnGeometryStreamsUnpadded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:1108]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ImageCreate.CancelMidStreamKeepsTruthfulPartialStats]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ImageCreate.CancelMidStreamKeepsTruthfulPartialStats]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ImageCreate.CancelMidStreamKeepsTruthfulPartialStats]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:1121]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ImageCreate.ReadFailureReportsCommittedPrefix]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ImageCreate.ReadFailureReportsCommittedPrefix]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ImageCreate.ReadFailureReportsCommittedPrefix]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:1142]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ImageCreate.ProgressCallbackPerChunk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ImageCreate.ProgressCallbackPerChunk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ImageCreate.ProgressCallbackPerChunk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:1163]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ImageCreate.EmptyDeviceIsEmptyStream]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ImageCreate.EmptyDeviceIsEmptyStream]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ImageCreate.EmptyDeviceIsEmptyStream]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:1180]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvmeHealth.PageFieldOffsets]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvmeHealth.PageFieldOffsets]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvmeHealth.PageFieldOffsets]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:51]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvmeHealth.FullFieldSurface]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvmeHealth.FullFieldSurface]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvmeHealth.FullFieldSurface]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:69]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvmeHealth.DataUnitsConvertedToBytes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvmeHealth.DataUnitsConvertedToBytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvmeHealth.DataUnitsConvertedToBytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:83]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvmeHealth.ShortBufferRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvmeHealth.ShortBufferRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvmeHealth.ShortBufferRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:100]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvmeHealth.NullRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvmeHealth.NullRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvmeHealth.NullRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:111]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvmeHealth.PercentageUsedIsU8Width]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvmeHealth.PercentageUsedIsU8Width]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvmeHealth.PercentageUsedIsU8Width]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:116]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Nvme.CliTempFractionIsCelsiusDecidegrees]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Nvme.CliTempFractionIsCelsiusDecidegrees]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Nvme.CliTempFractionIsCelsiusDecidegrees]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:133]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.SmartCommandListMatchesImplementation]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.SmartCommandListMatchesImplementation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.SmartCommandListMatchesImplementation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:167]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.Attr197Raw3IsCaution]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.Attr197Raw3IsCaution]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.Attr197Raw3IsCaution]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:37]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.ValueBelowVendorThresholdIsBad]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.ValueBelowVendorThresholdIsBad]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.ValueBelowVendorThresholdIsBad]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:48]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.Attr5RawAllFFSkipsCautionCheck]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.Attr5RawAllFFSkipsCautionCheck]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.Attr5RawAllFFSkipsCautionCheck]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:63]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.DuplicateAttrIdYieldsUnknown]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.DuplicateAttrIdYieldsUnknown]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.DuplicateAttrIdYieldsUnknown]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:78]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.NoGradedSignalYieldsUnknown]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.NoGradedSignalYieldsUnknown]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.NoGradedSignalYieldsUnknown]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:88]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.SsdAnyAttrBelowThresholdIsBad]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.SsdAnyAttrBelowThresholdIsBad]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.SsdAnyAttrBelowThresholdIsBad]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:107]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.TemperatureNeverAffectsTheDiskGrade]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.TemperatureNeverAffectsTheDiskGrade]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.TemperatureNeverAffectsTheDiskGrade]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:120]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.SsdLifeFamilyGatesAndBoundaries]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.SsdLifeFamilyGatesAndBoundaries]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.SsdLifeFamilyGatesAndBoundaries]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:145]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.SsdLifeBelowVendorThresholdIsBad]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.SsdLifeBelowVendorThresholdIsBad]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.SsdLifeBelowVendorThresholdIsBad]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:193]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.SsdLifeInvertFormReads100MinusRaw]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.SsdLifeInvertFormReads100MinusRaw]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.SsdLifeInvertFormReads100MinusRaw]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:216]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.WdcAttr230LifeFromRawByte]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.WdcAttr230LifeFromRawByte]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.WdcAttr230LifeFromRawByte]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:239]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.Attr1RuleIsSandForceGated]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.Attr1RuleIsSandForceGated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.Attr1RuleIsSandForceGated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:273]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.FailingBitForcesDiskBadButAttrGradesFollowOg]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.FailingBitForcesDiskBadButAttrGradesFollowOg]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.FailingBitForcesDiskBadButAttrGradesFollowOg]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:309]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.NvmeGradeMapping]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.NvmeGradeMapping]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.NvmeGradeMapping]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:327]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.ModelSaysSsdOcList]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.ModelSaysSsdOcList]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.ModelSaysSsdOcList]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:366]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.ClassifyFamilyDecodedVendors]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.ClassifyFamilyDecodedVendors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.ClassifyFamilyDecodedVendors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:384]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.ClassifyFamilyAttributeSignatures]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.ClassifyFamilyAttributeSignatures]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.ClassifyFamilyAttributeSignatures]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:404]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.ClassifyFamilyFormsAndFallbacks]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.ClassifyFamilyFormsAndFallbacks]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.ClassifyFamilyFormsAndFallbacks]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:421]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.MakeGradeInputsAppliesOgDefaults]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.MakeGradeInputsAppliesOgDefaults]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.MakeGradeInputsAppliesOgDefaults]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:444]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.GradeNames]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.GradeNames]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.GradeNames]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:461]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.CsmiRequestMatchesOgWireFormat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.CsmiRequestMatchesOgWireFormat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.CsmiRequestMatchesOgWireFormat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:116]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.CsmiIdentifyCarriesZeroLbaAndSmartCarriesMagic]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.CsmiIdentifyCarriesZeroLbaAndSmartCarriesMagic]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.CsmiIdentifyCarriesZeroLbaAndSmartCarriesMagic]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:158]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.CsmiBuilderRejectsUndersizedBuffer]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.CsmiBuilderRejectsUndersizedBuffer]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.CsmiBuilderRejectsUndersizedBuffer]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:175]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.CsmiResponseDataExtractedAtOgOffset]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.CsmiResponseDataExtractedAtOgOffset]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.CsmiResponseDataExtractedAtOgOffset]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:181]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.CsmiTransportUnavailableClassifiesTheOgErrorSet]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.CsmiTransportUnavailableClassifiesTheOgErrorSet]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.CsmiTransportUnavailableClassifiesTheOgErrorSet]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:195]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.CsmiTransportFallsBack]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.CsmiTransportFallsBack]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.CsmiTransportFallsBack]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:205]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.CsmiReadsAttributes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.CsmiReadsAttributes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.CsmiReadsAttributes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:228]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.SmartChainEmptyIsBadParameter]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.SmartChainEmptyIsBadParameter]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.SmartChainEmptyIsBadParameter]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:256]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.SmartAttrNameTableCoversTheCuratedIds]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Health.SmartAttrNameTableCoversTheCuratedIds]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.SmartAttrNameTableCoversTheCuratedIds]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:261]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.SingleRunDecode]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RunList.SingleRunDecode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.SingleRunDecode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:9]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.NegativeDelta]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RunList.NegativeDelta]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.NegativeDelta]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:22]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.SparseHoleEmitsExtent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RunList.SparseHoleEmitsExtent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.SparseHoleEmitsExtent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:35]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.ContiguousCoalesceRequiresLogicalContiguityToo]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RunList.ContiguousCoalesceRequiresLogicalContiguityToo]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.ContiguousCoalesceRequiresLogicalContiguityToo]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:67]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.SparseRun]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RunList.SparseRun]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.SparseRun]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:83]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.TerminatesAndRejectsTruncated]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RunList.TerminatesAndRejectsTruncated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.TerminatesAndRejectsTruncated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:105]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.TornAfterValidRunsKeepsPrefix]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RunList.TornAfterValidRunsKeepsPrefix]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.TornAfterValidRunsKeepsPrefix]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:122]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.AllGarbageYieldsNothing]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RunList.AllGarbageYieldsNothing]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.AllGarbageYieldsNothing]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:154]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.SparseFirstAcceptedAndRegistered]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RunList.SparseFirstAcceptedAndRegistered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.SparseFirstAcceptedAndRegistered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:167]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.ZeroLengthRunRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RunList.ZeroLengthRunRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.ZeroLengthRunRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:181]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.MaxClusterGuard]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RunList.MaxClusterGuard]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.MaxClusterGuard]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:187]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.EncodeDecodeRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RunList.EncodeDecodeRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.EncodeDecodeRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:194]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.SignedWidthEdgeCases]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RunList.SignedWidthEdgeCases]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.SignedWidthEdgeCases]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:213]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.ExtentCoalesce]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RunList.ExtentCoalesce]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.ExtentCoalesce]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:231]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.Int64MinOffsetRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RunList.Int64MinOffsetRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.Int64MinOffsetRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:245]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crc32.KnownVectors]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Crc32.KnownVectors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crc32.KnownVectors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:49]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.ValidTable]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.ValidTable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.ValidTable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:56]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.BadHeaderCrcDetected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.BadHeaderCrcDetected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.BadHeaderCrcDetected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:124]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.DeadPrimaryHeaderSalvagesOwnEntries]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.DeadPrimaryHeaderSalvagesOwnEntries]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.DeadPrimaryHeaderSalvagesOwnEntries]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:628]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.ProtectiveOnlyVolumeRecoversFromBackup]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.ProtectiveOnlyVolumeRecoversFromBackup]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.ProtectiveOnlyVolumeRecoversFromBackup]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:647]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.EntryLastLbaBeforeFirstLbaSkippedAndCounted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.EntryLastLbaBeforeFirstLbaSkippedAndCounted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.EntryLastLbaBeforeFirstLbaSkippedAndCounted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:679]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.EntrySpanOverflowBoundary]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.EntrySpanOverflowBoundary]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.EntrySpanOverflowBoundary]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:698]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.HostileSalvageHeaderEntriesOutsideDiskRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.HostileSalvageHeaderEntriesOutsideDiskRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.HostileSalvageHeaderEntriesOutsideDiskRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:714]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.CanonicalTypeGuidConstantsMatchRealDiskBytes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.CanonicalTypeGuidConstantsMatchRealDiskBytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.CanonicalTypeGuidConstantsMatchRealDiskBytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:765]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:817]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.EspMsrLinuxBiosbootLdmClassification]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.EspMsrLinuxBiosbootLdmClassification]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.EspMsrLinuxBiosbootLdmClassification]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:828]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.OgGateSetHealthyHeaderTrusted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.OgGateSetHealthyHeaderTrusted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.OgGateSetHealthyHeaderTrusted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:947]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.HeaderSizeNot92Rejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.HeaderSizeNot92Rejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.HeaderSizeNot92Rejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:960]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.ReservedNonzeroRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.ReservedNonzeroRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.ReservedNonzeroRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:981]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.WrongRevisionRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.WrongRevisionRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.WrongRevisionRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:992]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.PrimaryCurrentLbaWrongRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.PrimaryCurrentLbaWrongRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.PrimaryCurrentLbaWrongRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:1005]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.BackupLbaWrongRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.BackupLbaWrongRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.BackupLbaWrongRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:1017]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.UsableRangeBeyondDeviceRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.UsableRangeBeyondDeviceRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.UsableRangeBeyondDeviceRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:1029]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.EntriesLbaInsideUsableRangeRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.EntriesLbaInsideUsableRangeRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.EntriesLbaInsideUsableRangeRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:1043]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.TruncatedImageFallsToLadderNotTrustedPrimary]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.TruncatedImageFallsToLadderNotTrustedPrimary]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.TruncatedImageFallsToLadderNotTrustedPrimary]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:1056]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.BackupProbedAtDeviceLastWhenPrimaryOk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.BackupProbedAtDeviceLastWhenPrimaryOk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.BackupProbedAtDeviceLastWhenPrimaryOk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:1085]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.CorruptBackupLbaFieldStillFindsBackup]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.CorruptBackupLbaFieldStillFindsBackup]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.CorruptBackupLbaFieldStillFindsBackup]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:1106]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.DeadLba1BackupRecoveryRegistered]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Gpt.DeadLba1BackupRecoveryRegistered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.DeadLba1BackupRecoveryRegistered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:1126]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Mbr.PlainWithExtended]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Mbr.PlainWithExtended]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Mbr.PlainWithExtended]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:157]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Mbr.LinuxExtended085LinksFollowTheChain]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Mbr.LinuxExtended085LinksFollowTheChain]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Mbr.LinuxExtended085LinksFollowTheChain]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:205]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Mbr.EbrLayoutAssignedAndPlainMbrStaysMbr]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Mbr.EbrLayoutAssignedAndPlainMbrStaysMbr]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Mbr.EbrLayoutAssignedAndPlainMbrStaysMbr]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:260]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Mbr.EbrCycleGuard]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Mbr.EbrCycleGuard]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Mbr.EbrCycleGuard]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:303]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Mbr.Extended085NotEmittedAsVolume]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Mbr.Extended085NotEmittedAsVolume]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Mbr.Extended085NotEmittedAsVolume]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:446]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Mbr.TornLinkKeepsPriorVolumes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Mbr.TornLinkKeepsPriorVolumes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Mbr.TornLinkKeepsPriorVolumes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:482]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Mbr.TornLinkStatusReported]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Mbr.TornLinkStatusReported]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Mbr.TornLinkStatusReported]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:531]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ebr.LinkType085WalksChain]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ebr.LinkType085WalksChain]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ebr.LinkType085WalksChain]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:346]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ebr.LinkType085SameBasisAs05]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ebr.LinkType085SameBasisAs05]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ebr.LinkType085SameBasisAs05]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:401]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.BoundsEnforced]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Slice.BoundsEnforced]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.BoundsEnforced]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:36]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.TailReadClampsAndZeroFills]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Slice.TailReadClampsAndZeroFills]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.TailReadClampsAndZeroFills]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:76]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.StartMustBeAligned]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Slice.StartMustBeAligned]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.StartMustBeAligned]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:99]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.RaggedTailAllowed]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Slice.RaggedTailAllowed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.RaggedTailAllowed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:106]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.GeometryRefreshAfterMediaChange]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Slice.GeometryRefreshAfterMediaChange]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.GeometryRefreshAfterMediaChange]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:219]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.GeometryRefreshRefusesInvalidatingChange]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Slice.GeometryRefreshRefusesInvalidatingChange]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.GeometryRefreshRefusesInvalidatingChange]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:249]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.CreateClampsOverlongDeclaredLength]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Slice.CreateClampsOverlongDeclaredLength]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.CreateClampsOverlongDeclaredLength]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:315]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.CreateStartBeyondDeviceStillRefused]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Slice.CreateStartBeyondDeviceStillRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.CreateStartBeyondDeviceStillRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:341]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.ExactFitNotClamped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Slice.ExactFitNotClamped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.ExactFitNotClamped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:352]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.OrchestratorTargetClampScansRaggedTable]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Slice.OrchestratorTargetClampScansRaggedTable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.OrchestratorTargetClampScansRaggedTable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:363]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DevMap.AddMergeSerialize]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DevMap.AddMergeSerialize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DevMap.AddMergeSerialize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:116]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DevMap.ModuleAttributionRecordedAndNewestWinsOnCoalesce]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DevMap.ModuleAttributionRecordedAndNewestWinsOnCoalesce]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DevMap.ModuleAttributionRecordedAndNewestWinsOnCoalesce]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:138]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DevMap.ModuleNotSerializedWireStaysTrio]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DevMap.ModuleNotSerializedWireStaysTrio]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DevMap.ModuleNotSerializedWireStaysTrio]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:172]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Storage.ParityDeltasListsNasExclusion]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Storage.ParityDeltasListsNasExclusion]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Storage.ParityDeltasListsNasExclusion]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:279]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Cached.HitMissAndBridging]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Cached.HitMissAndBridging]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Cached.HitMissAndBridging]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_cached.cpp:36]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Cached.BoundsReject]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Cached.BoundsReject]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Cached.BoundsReject]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_cached.cpp:61]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Cached.FailedBlocksAreNotCachedForever]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Cached.FailedBlocksAreNotCachedForever]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Cached.FailedBlocksAreNotCachedForever]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_cached.cpp:68]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Cached.ScanCacheBudgetIsTheRegisteredDivergence]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Cached.ScanCacheBudgetIsTheRegisteredDivergence]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Cached.ScanCacheBudgetIsTheRegisteredDivergence]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_cached.cpp:109]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.AlignedRequestIsTheFastPath]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DirectAlign.AlignedRequestIsTheFastPath]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.AlignedRequestIsTheFastPath]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:15]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.FourKMultipleNotSixtyFourKTakesTheEnvelope]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DirectAlign.FourKMultipleNotSixtyFourKTakesTheEnvelope]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.FourKMultipleNotSixtyFourKTakesTheEnvelope]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:26]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.UnalignedOffsetAndLengthEnvelope]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DirectAlign.UnalignedOffsetAndLengthEnvelope]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.UnalignedOffsetAndLengthEnvelope]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:39]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.MultiBlockEnvelopeCoversTheRequest]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DirectAlign.MultiBlockEnvelopeCoversTheRequest]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.MultiBlockEnvelopeCoversTheRequest]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:50]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.DeviceTailClampIsTheDocumentedException]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DirectAlign.DeviceTailClampIsTheDocumentedException]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.DeviceTailClampIsTheDocumentedException]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:60]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.OneMiBWriterChunkAtAlignedOffsetStaysDirect]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DirectAlign.OneMiBWriterChunkAtAlignedOffsetStaysDirect]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.OneMiBWriterChunkAtAlignedOffsetStaysDirect]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:73]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.OutOfBoundsAndEmptyRequestsFail]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DirectAlign.OutOfBoundsAndEmptyRequestsFail]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.OutOfBoundsAndEmptyRequestsFail]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:84]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.IssuedLengthAlwaysSixtyFourKMultipleExceptTail]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DirectAlign.IssuedLengthAlwaysSixtyFourKMultipleExceptTail]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.IssuedLengthAlwaysSixtyFourKMultipleExceptTail]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:90]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.UnifiedEnvelopeExhaustivePropertyBothDevices]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DirectAlign.UnifiedEnvelopeExhaustivePropertyBothDevices]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.UnifiedEnvelopeExhaustivePropertyBothDevices]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:104]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.CleanReadPassesThrough]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Retry.CleanReadPassesThrough]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.CleanReadPassesThrough]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:53]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.FailModeReportsExhaustedAndMapsRange]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Retry.FailModeReportsExhaustedAndMapsRange]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.FailModeReportsExhaustedAndMapsRange]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:65]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.SkipModeZeroFillsOnlyBadSectors]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Retry.SkipModeZeroFillsOnlyBadSectors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.SkipModeZeroFillsOnlyBadSectors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:78]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.UnalignedBadRangeRoundsToWholeSectors]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Retry.UnalignedBadRangeRoundsToWholeSectors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.UnalignedBadRangeRoundsToWholeSectors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:101]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.BacktrackReadSalvagesPartialSector]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Retry.BacktrackReadSalvagesPartialSector]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.BacktrackReadSalvagesPartialSector]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:146]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.BacktrackOffReproducesLegacyExhausted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Retry.BacktrackOffReproducesLegacyExhausted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.BacktrackOffReproducesLegacyExhausted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:165]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.BacktrackStaysAlignedOn4Kn]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Retry.BacktrackStaysAlignedOn4Kn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.BacktrackStaysAlignedOn4Kn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:177]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.BoundsAreEnforcedBeforeInner]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Retry.BoundsAreEnforcedBeforeInner]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.BoundsAreEnforcedBeforeInner]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:191]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.DeterministicFailureNotRetried]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Retry.DeterministicFailureNotRetried]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.DeterministicFailureNotRetried]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:202]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.GeometryForwarded]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Retry.GeometryForwarded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.GeometryForwarded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:212]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.ThreadStampAttributedToRecordedRange]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Retry.ThreadStampAttributedToRecordedRange]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.ThreadStampAttributedToRecordedRange]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:221]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.OwnerFallbackCoversUnstampedWorkerThreads]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Retry.OwnerFallbackCoversUnstampedWorkerThreads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.OwnerFallbackCoversUnstampedWorkerThreads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:241]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Errmod.GuardSetsAndRestoresTheThreadStamp]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Errmod.GuardSetsAndRestoresTheThreadStamp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Errmod.GuardSetsAndRestoresTheThreadStamp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:272]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pool.RunsAllItemsExactlyOnce]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Pool.RunsAllItemsExactlyOnce]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pool.RunsAllItemsExactlyOnce]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:22]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pool.AbortStopsWork]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Pool.AbortStopsWork]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pool.AbortStopsWork]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:36]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pool.ExternalCancelTokenAdapter]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Pool.ExternalCancelTokenAdapter]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pool.ExternalCancelTokenAdapter]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:50]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pool.ExceptionsNeverCrossThreadEdge]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Pool.ExceptionsNeverCrossThreadEdge]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pool.ExceptionsNeverCrossThreadEdge]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:58]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pool.WaitIdleAndPending]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Pool.WaitIdleAndPending]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pool.WaitIdleAndPending]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:71]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pool.CancelPoolWatcherDrainsQueueWithinOneTick]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Pool.CancelPoolWatcherDrainsQueueWithinOneTick]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pool.CancelPoolWatcherDrainsQueueWithinOneTick]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:273]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pool.CancelPoolWatcherNullTokenIsInert]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Pool.CancelPoolWatcherNullTokenIsInert]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pool.CancelPoolWatcherNullTokenIsInert]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:312]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ParallelCarve.MatchesSingleThreaded]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ParallelCarve.MatchesSingleThreaded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ParallelCarve.MatchesSingleThreaded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:159]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ParallelRemnants.SameRecordsAsSequential]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ParallelRemnants.SameRecordsAsSequential]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ParallelRemnants.SameRecordsAsSequential]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:249]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NodeTree.AddRootsAndFiles]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NodeTree.AddRootsAndFiles]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NodeTree.AddRootsAndFiles]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_node_tree.cpp:19]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NodeTree.HardLinkSharesProps]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NodeTree.HardLinkSharesProps]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NodeTree.HardLinkSharesProps]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_node_tree.cpp:41]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NodeTree.ExtentDedupIndex]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NodeTree.ExtentDedupIndex]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NodeTree.ExtentDedupIndex]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_node_tree.cpp:54]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NodeTree.ResetFreesClean]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NodeTree.ResetFreesClean]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NodeTree.ResetFreesClean]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_node_tree.cpp:67]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.MonotonicMaxNeverDowngrades]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.MonotonicMaxNeverDowngrades]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.MonotonicMaxNeverDowngrades]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:59]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.Grade2Sticky]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.Grade2Sticky]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.Grade2Sticky]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:74]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.DefaultGradeIsAverageAtEmit]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.DefaultGradeIsAverageAtEmit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.DefaultGradeIsAverageAtEmit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:88]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.SizeFloorIs1]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.SizeFloorIs1]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.SizeFloorIs1]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:100]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.EmailCategoryFastPath]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.EmailCategoryFastPath]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.EmailCategoryFastPath]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:112]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.RefsAndBtrfsDeletedFilesGradeHigh]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.RefsAndBtrfsDeletedFilesGradeHigh]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.RefsAndBtrfsDeletedFilesGradeHigh]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:127]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.RefsExistingStillHighAndSmallStillLow]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.RefsExistingStillHighAndSmallStillLow]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.RefsExistingStillHighAndSmallStillLow]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:150]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.ExistingFilesHighDeletedNot]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.ExistingFilesHighDeletedNot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.ExistingFilesHighDeletedNot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:167]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.SmallFileUnder1024High]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.SmallFileUnder1024High]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.SmallFileUnder1024High]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:179]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.GaugePopulationIsMemberSet]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.GaugePopulationIsMemberSet]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.GaugePopulationIsMemberSet]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:195]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.ScoreFormulaRoundsLikeOg]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.ScoreFormulaRoundsLikeOg]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.ScoreFormulaRoundsLikeOg]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:218]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.QSDENotTheNtfsIndexCount]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.QSDENotTheNtfsIndexCount]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.QSDENotTheNtfsIndexCount]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:249]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.NtfsDeviceIndexRuleUpgrades]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.NtfsDeviceIndexRuleUpgrades]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.NtfsDeviceIndexRuleUpgrades]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:276]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.ExtensionTableGatesDeepValidation]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.ExtensionTableGatesDeepValidation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.ExtensionTableGatesDeepValidation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:293]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.DotlessNamesSkipTwinDetection]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.DotlessNamesSkipTwinDetection]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.DotlessNamesSkipTwinDetection]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:335]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.RefsDeletedFilesGradeHigh]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.RefsDeletedFilesGradeHigh]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.RefsDeletedFilesGradeHigh]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:353]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.BtrfsDeletedFilesGradeHigh]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.BtrfsDeletedFilesGradeHigh]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.BtrfsDeletedFilesGradeHigh]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:366]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.RefsExistingStillHigh]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.RefsExistingStillHigh]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.RefsExistingStillHigh]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:377]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.TwinAddressFlagsBoth]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.TwinAddressFlagsBoth]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.TwinAddressFlagsBoth]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:389]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.TableMembershipAndCount]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.TableMembershipAndCount]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.TableMembershipAndCount]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:418]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.TwinKeyFat16StillShifts]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.TwinKeyFat16StillShifts]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.TwinKeyFat16StillShifts]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:479]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.TwinKeyFat32DoesNotShift]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.TwinKeyFat32DoesNotShift]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.TwinKeyFat32DoesNotShift]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:493]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.TwinKeyExfatDoesNotShift]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.TwinKeyExfatDoesNotShift]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.TwinKeyExfatDoesNotShift]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:507]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.TwinKeyFat12DoesNotShift]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.TwinKeyFat12DoesNotShift]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.TwinKeyFat12DoesNotShift]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:519]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.TwinKeyExt4NtfsNeverShift]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.TwinKeyExt4NtfsNeverShift]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.TwinKeyExt4NtfsNeverShift]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:531]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.RegradeOnlyGrade1]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.RegradeOnlyGrade1]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.RegradeOnlyGrade1]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:551]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.AssistantCountersMixedFixture]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.AssistantCountersMixedFixture]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.AssistantCountersMixedFixture]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:571]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.SessionRoundTripPreservesGrade]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.SessionRoundTripPreservesGrade]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.SessionRoundTripPreservesGrade]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:601]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.DeepReadZeroedFirstSectorGradesLow]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.DeepReadZeroedFirstSectorGradesLow]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.DeepReadZeroedFirstSectorGradesLow]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:778]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.DeepRead0xFFHeadGradesLow]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.DeepRead0xFFHeadGradesLow]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.DeepRead0xFFHeadGradesLow]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:797]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.DeepReadOwnLastSectorTailZeroGradesHigh]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.DeepReadOwnLastSectorTailZeroGradesHigh]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.DeepReadOwnLastSectorTailZeroGradesHigh]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:813]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.DeepReadPrevSectorPaddingNotHigh]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.DeepReadPrevSectorPaddingNotHigh]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.DeepReadPrevSectorPaddingNotHigh]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:836]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.DeepReadSector0StartFileNotDemotedByHead]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.DeepReadSector0StartFileNotDemotedByHead]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.DeepReadSector0StartFileNotDemotedByHead]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:858]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.DeepReadShiftedKindSkippedLikeOgFailedRead]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.DeepReadShiftedKindSkippedLikeOgFailedRead]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.DeepReadShiftedKindSkippedLikeOgFailedRead]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:877]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.DeepReadSkipsOverlapFlagged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.DeepReadSkipsOverlapFlagged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.DeepReadSkipsOverlapFlagged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:896]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.RegradeRunsFullTreeOnce]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.RegradeRunsFullTreeOnce]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.RegradeRunsFullTreeOnce]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:921]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.DeepReadReaderAbsentChangesNothing]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.DeepReadReaderAbsentChangesNothing]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.DeepReadReaderAbsentChangesNothing]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:943]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.RawTailZeroTicksForcedProbePopulation]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.RawTailZeroTicksForcedProbePopulation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.RawTailZeroTicksForcedProbePopulation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:960]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.RawTailZeroSizeArmBoundary]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.RawTailZeroSizeArmBoundary]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.RawTailZeroSizeArmBoundary]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:996]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.RawCountSemanticsRegistered]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.RawCountSemanticsRegistered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.RawCountSemanticsRegistered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:1015]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.TwinKeyExt2FamilyShifts]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Grading.TwinKeyExt2FamilyShifts]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.TwinKeyExt2FamilyShifts]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:1052]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.ContainsAssistantStatistics]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Report.ContainsAssistantStatistics]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.ContainsAssistantStatistics]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:679]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.ScanAnalysisCarriesPerFormat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Report.ScanAnalysisCarriesPerFormat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.ScanAnalysisCarriesPerFormat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:711]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.EscapingStrictJson]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Report.EscapingStrictJson]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.EscapingStrictJson]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:99]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.SchemaShapeAndDeterminism]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Report.SchemaShapeAndDeterminism]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.SchemaShapeAndDeterminism]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:121]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.WriterOutcomesAndHashHonesty]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Report.WriterOutcomesAndHashHonesty]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.WriterOutcomesAndHashHonesty]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:180]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.MultiExtentSpannedVolumeRefused]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Report.MultiExtentSpannedVolumeRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.MultiExtentSpannedVolumeRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:255]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.DestFullFastFailStopsAttempting]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Report.DestFullFastFailStopsAttempting]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.DestFullFastFailStopsAttempting]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:313]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.DestFullPreCheckRefusesUpfront]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Report.DestFullPreCheckRefusesUpfront]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.DestFullPreCheckRefusesUpfront]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:354]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.ReadErrorOutcomeMarksTruncatedPartials]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Report.ReadErrorOutcomeMarksTruncatedPartials]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.ReadErrorOutcomeMarksTruncatedPartials]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:388]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.EmitsCiSchemaSample]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Report.EmitsCiSchemaSample]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.EmitsCiSchemaSample]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:482]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.FailedRowsCarryTreeClassification]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Report.FailedRowsCarryTreeClassification]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.FailedRowsCarryTreeClassification]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:560]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.FailedBytesSplitFromTotal]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Report.FailedBytesSplitFromTotal]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.FailedBytesSplitFromTotal]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:610]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.RawCategoryNameClassification]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Report.RawCategoryNameClassification]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.RawCategoryNameClassification]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:651]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.TreeClassIsAdditiveWithinSchemaV1]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Report.TreeClassIsAdditiveWithinSchemaV1]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.TreeClassIsAdditiveWithinSchemaV1]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:676]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.ScopeDeltaRegisteredWithRevisitTrigger]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Enumerate.ScopeDeltaRegisteredWithRevisitTrigger]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.ScopeDeltaRegisteredWithRevisitTrigger]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_win.cpp:21]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.BusyDiskMetadataRetryShipsInQueryDisk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Enumerate.BusyDiskMetadataRetryShipsInQueryDisk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.BusyDiskMetadataRetryShipsInQueryDisk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_win.cpp:36]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.ScopeContractDocumentedInHeader]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Enumerate.ScopeContractDocumentedInHeader]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.ScopeContractDocumentedInHeader]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_win.cpp:56]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.TrimChipOptInShipsInAbiAndCli]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Enumerate.TrimChipOptInShipsInAbiAndCli]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.TrimChipOptInShipsInAbiAndCli]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_win.cpp:73]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.SetupApiMerge]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Enumerate.SetupApiMerge]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.SetupApiMerge]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_merge.cpp:44]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.DedupByInstancePath]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Enumerate.DedupByInstancePath]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.DedupByInstancePath]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_merge.cpp:73]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.MergeWithEmptyInterfaceStackKeepsLoop]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Enumerate.MergeWithEmptyInterfaceStackKeepsLoop]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.MergeWithEmptyInterfaceStackKeepsLoop]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_merge.cpp:96]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.MergeWithEmptyLoopTakesAllInterfaces]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Enumerate.MergeWithEmptyLoopTakesAllInterfaces]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.MergeWithEmptyLoopTakesAllInterfaces]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_merge.cpp:109]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.DeviceDescriptorFieldsDecode]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Enumerate.DeviceDescriptorFieldsDecode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.DeviceDescriptorFieldsDecode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_merge.cpp:157]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.DeviceDescriptorBoundedAndAbsent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Enumerate.DeviceDescriptorBoundedAndAbsent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.DeviceDescriptorBoundedAndAbsent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_merge.cpp:181]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.DeviceIdWwnDecode]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Enumerate.DeviceIdWwnDecode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.DeviceIdWwnDecode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_merge.cpp:198]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.NonresidentAttrlistOnMft]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.NonresidentAttrlistOnMft]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.NonresidentAttrlistOnMft]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:856]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.NestedAttrlistChase]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.NestedAttrlistChase]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.NestedAttrlistChase]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:905]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.ResidentAdsCarriesContent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.ResidentAdsCarriesContent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.ResidentAdsCarriesContent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:969]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.AdsAttachesToFirstLink]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.AdsAttachesToFirstLink]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.AdsAttachesToFirstLink]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1010]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.UsnJrnlHarvest]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.UsnJrnlHarvest]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.UsnJrnlHarvest]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1114]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.UsnJrnlSparseV3Resync]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.UsnJrnlSparseV3Resync]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.UsnJrnlSparseV3Resync]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1222]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.ShiftedHeaderSalvage]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.ShiftedHeaderSalvage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.ShiftedHeaderSalvage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1361]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.EndToEndWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.EndToEndWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.EndToEndWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1398]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.OriginLcnCarriedFromFirstDataRun]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.OriginLcnCarriedFromFirstDataRun]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.OriginLcnCarriedFromFirstDataRun]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1548]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.HardLinkSingleLinkMatchesOg]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.HardLinkSingleLinkMatchesOg]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.HardLinkSingleLinkMatchesOg]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1674]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.HardLinkFanOutOptIn]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.HardLinkFanOutOptIn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.HardLinkFanOutOptIn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1713]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.ParentAnchorIsMaskedFileNameParentRecord]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.ParentAnchorIsMaskedFileNameParentRecord]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.ParentAnchorIsMaskedFileNameParentRecord]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1747]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.HardLinkFanOutClonesKeepShortName]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.HardLinkFanOutClonesKeepShortName]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.HardLinkFanOutClonesKeepShortName]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1830]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RecycleBinMapping]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.RecycleBinMapping]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RecycleBinMapping]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1882]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RemnantScanFindsDetachedRecord]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.RemnantScanFindsDetachedRecord]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RemnantScanFindsDetachedRecord]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1979]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.ViabilityProbeUsesDataSize]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.ViabilityProbeUsesDataSize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.ViabilityProbeUsesDataSize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2007]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.SiTimeSlotsMatchOgSurfacedSet]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.SiTimeSlotsMatchOgSurfacedSet]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.SiTimeSlotsMatchOgSurfacedSet]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2062]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.ViabilityBoundaryExactFastPath]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.ViabilityBoundaryExactFastPath]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.ViabilityBoundaryExactFastPath]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2150]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.TornRunlistKeepsPrefixAndGradesPartialRisk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.TornRunlistKeepsPrefixAndGradesPartialRisk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.TornRunlistKeepsPrefixAndGradesPartialRisk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2199]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.ExtensionRecordsNotCountedDamaged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.ExtensionRecordsNotCountedDamaged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.ExtensionRecordsNotCountedDamaged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2251]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.CompressedDataRecoversRealContent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.CompressedDataRecoversRealContent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.CompressedDataRecoversRealContent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2295]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.Lznt1MsXcaDocumentedVector]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.Lznt1MsXcaDocumentedVector]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.Lznt1MsXcaDocumentedVector]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2410]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RemnantScanReconstructsDataRunlist]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.RemnantScanReconstructsDataRunlist]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RemnantScanReconstructsDataRunlist]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2440]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RemnantScanReconstructsResidentData]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.RemnantScanReconstructsResidentData]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RemnantScanReconstructsResidentData]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2476]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RemnantScanCompressedStreamStaysMetadataOnly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.RemnantScanCompressedStreamStaysMetadataOnly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RemnantScanCompressedStreamStaysMetadataOnly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2506]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.OverwrittenHonestModeKeepsAndGrades]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.OverwrittenHonestModeKeepsAndGrades]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.OverwrittenHonestModeKeepsAndGrades]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2540]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.HardLinkDestroyRuleInteraction]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.HardLinkDestroyRuleInteraction]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.HardLinkDestroyRuleInteraction]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2585]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BootNoPow2RecordSizeAcceptedAtDecodedValue]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.BootNoPow2RecordSizeAcceptedAtDecodedValue]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BootNoPow2RecordSizeAcceptedAtDecodedValue]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2751]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BootRecordSizeZeroDefaultsTo1024]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.BootRecordSizeZeroDefaultsTo1024]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BootRecordSizeZeroDefaultsTo1024]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2769]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BootCibZeroDefaultsTo4096IndexBlocks]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.BootCibZeroDefaultsTo4096IndexBlocks]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BootCibZeroDefaultsTo4096IndexBlocks]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2805]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BootCprZeroWithValidPrimaryDoesNotConsultBackup]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.BootCprZeroWithValidPrimaryDoesNotConsultBackup]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BootCprZeroWithValidPrimaryDoesNotConsultBackup]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2820]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BootRecordShiftFormDecodesAt1024]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.BootRecordShiftFormDecodesAt1024]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BootRecordShiftFormDecodesAt1024]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2838]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BootNegativeExponentOverflowRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.BootNegativeExponentOverflowRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BootNegativeExponentOverflowRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2856]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.OddSectorRecordWalkScans]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.OddSectorRecordWalkScans]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.OddSectorRecordWalkScans]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2877]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.OversizePositiveRecordOpensAndWalks]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.OversizePositiveRecordOpensAndWalks]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.OversizePositiveRecordOpensAndWalks]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2977]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BootSpcShiftDecodeConventionPinned]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.BootSpcShiftDecodeConventionPinned]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BootSpcShiftDecodeConventionPinned]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3010]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BootUsableLargeRecordSizeKept]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.BootUsableLargeRecordSizeKept]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BootUsableLargeRecordSizeKept]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3055]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RemnantReachableUnderTornDbr]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.RemnantReachableUnderTornDbr]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RemnantReachableUnderTornDbr]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3068]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.MirrorCoversRunlistSize]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.MirrorCoversRunlistSize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.MirrorCoversRunlistSize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3171]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.MirrorResidentDataRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.MirrorResidentDataRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.MirrorResidentDataRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3212]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.UserFileNamedDollarNotHidden]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.UserFileNamedDollarNotHidden]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.UserFileNamedDollarNotHidden]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3235]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.MainWalkKeepsOverwrittenDeleted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.MainWalkKeepsOverwrittenDeleted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.MainWalkKeepsOverwrittenDeleted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3517]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.GuidDumpPassRecoversReusedRecord]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.GuidDumpPassRecoversReusedRecord]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.GuidDumpPassRecoversReusedRecord]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3555]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.OriginLcnCarriedOnGuidDumpRecord]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.OriginLcnCarriedOnGuidDumpRecord]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.OriginLcnCarriedOnGuidDumpRecord]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3573]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.GuidDumpDestroyRules]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.GuidDumpDestroyRules]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.GuidDumpDestroyRules]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3594]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.SizeZeroRescueFromMftCopy]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.SizeZeroRescueFromMftCopy]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.SizeZeroRescueFromMftCopy]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3612]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.SizeZeroRescueSameNameMerge]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.SizeZeroRescueSameNameMerge]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.SizeZeroRescueSameNameMerge]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3630]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.GuidDumpStrideIsRecordAligned]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.GuidDumpStrideIsRecordAligned]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.GuidDumpStrideIsRecordAligned]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3650]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.GuidDumpLargeRecordParsedFully]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.GuidDumpLargeRecordParsedFully]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.GuidDumpLargeRecordParsedFully]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3685]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.MainWalkKeepsSystemFlaggedUserFile]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.MainWalkKeepsSystemFlaggedUserFile]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.MainWalkKeepsSystemFlaggedUserFile]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3795]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.OrphanIndxBlockHarvestsEntries]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.OrphanIndxBlockHarvestsEntries]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.OrphanIndxBlockHarvestsEntries]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3954]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.IndxHiddenSystemEntryRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.IndxHiddenSystemEntryRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.IndxHiddenSystemEntryRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3994]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.IndxSlackNeverHarvested]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.IndxSlackNeverHarvested]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.IndxSlackNeverHarvested]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4018]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RemnantDirRecordHarvestsIndexRootChildren]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.RemnantDirRecordHarvestsIndexRootChildren]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RemnantDirRecordHarvestsIndexRootChildren]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4049]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.AdsDroppedInParityMode]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.AdsDroppedInParityMode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.AdsDroppedInParityMode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4089]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.AdsEmittedByDefaultSuperset]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.AdsEmittedByDefaultSuperset]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.AdsEmittedByDefaultSuperset]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4137]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.AttributesFromSIWhenAvailable]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.AttributesFromSIWhenAvailable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.AttributesFromSIWhenAvailable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4159]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.AttributesFallBackToFileNameWhenSIAbsent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.AttributesFallBackToFileNameWhenSIAbsent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.AttributesFallBackToFileNameWhenSIAbsent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4201]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.AttributesPreserveOfflineBit]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.AttributesPreserveOfflineBit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.AttributesPreserveOfflineBit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4241]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.ShortNameFieldRoundtrips]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.ShortNameFieldRoundtrips]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.ShortNameFieldRoundtrips]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4281]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.UsnHarvestTrueDeleteReason0x80020000]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.UsnHarvestTrueDeleteReason0x80020000]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.UsnHarvestTrueDeleteReason0x80020000]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4382]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.UsnHarvestRejectsCompressionChangeOnly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.UsnHarvestRejectsCompressionChangeOnly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.UsnHarvestRejectsCompressionChangeOnly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4446]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.UsnHarvestCapMinesTail]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.UsnHarvestCapMinesTail]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.UsnHarvestCapMinesTail]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4507]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.DiskMapFreeCellsFromRealBitmap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.DiskMapFreeCellsFromRealBitmap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.DiskMapFreeCellsFromRealBitmap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4648]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.MultiHopAttrlistOgParityStopsAtDepth1]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.MultiHopAttrlistOgParityStopsAtDepth1]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.MultiHopAttrlistOgParityStopsAtDepth1]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4773]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.DeletedAttrlistChaseFollowsChildrenAtDepth1]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.DeletedAttrlistChaseFollowsChildrenAtDepth1]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.DeletedAttrlistChaseFollowsChildrenAtDepth1]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4827]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.TornPrimaryBackupBootScans]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.TornPrimaryBackupBootScans]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.TornPrimaryBackupBootScans]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4891]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.TornSignatureWithIntactBackupStillFallsBack]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.TornSignatureWithIntactBackupStillFallsBack]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.TornSignatureWithIntactBackupStillFallsBack]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4928]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.TornMftmirrLcnScansWithMirrorSkipped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.TornMftmirrLcnScansWithMirrorSkipped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.TornMftmirrLcnScansWithMirrorSkipped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4951]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.MftLcnProductFormBoundaryAndHardening]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.MftLcnProductFormBoundaryAndHardening]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.MftLcnProductFormBoundaryAndHardening]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4985]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.TornPrimaryShapeBackupBootScans]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.TornPrimaryShapeBackupBootScans]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.TornPrimaryShapeBackupBootScans]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5021]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BackupBadFallsBackToRefusal]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.BackupBadFallsBackToRefusal]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BackupBadFallsBackToRefusal]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5042]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BootFromBackupFlagged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.BootFromBackupFlagged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BootFromBackupFlagged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5058]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.SpoofedOemRejectedEarly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.SpoofedOemRejectedEarly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.SpoofedOemRejectedEarly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5070]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RecognitionNonPow2SpcDispatches]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.RecognitionNonPow2SpcDispatches]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RecognitionNonPow2SpcDispatches]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5111]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RecognitionReservedBandNonzeroDeclinesAtDispatch]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.RecognitionReservedBandNonzeroDeclinesAtDispatch]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RecognitionReservedBandNonzeroDeclinesAtDispatch]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5130]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RecognitionUnusedTailBandNonzeroDeclinesAtDispatch]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.RecognitionUnusedTailBandNonzeroDeclinesAtDispatch]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RecognitionUnusedTailBandNonzeroDeclinesAtDispatch]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5143]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RecognitionUnsweptBandsStayAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.RecognitionUnsweptBandsStayAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RecognitionUnsweptBandsStayAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5155]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RecognitionRecordSizeShapeGateDeclinesAtDispatch]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.RecognitionRecordSizeShapeGateDeclinesAtDispatch]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RecognitionRecordSizeShapeGateDeclinesAtDispatch]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5178]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RecognitionIndexBlockShapeGateDeclinesAtDispatch]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.RecognitionIndexBlockShapeGateDeclinesAtDispatch]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RecognitionIndexBlockShapeGateDeclinesAtDispatch]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5194]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RecognitionShapeGateZeroDefaultsPassAtDispatch]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.RecognitionShapeGateZeroDefaultsPassAtDispatch]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RecognitionShapeGateZeroDefaultsPassAtDispatch]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5209]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RecycleBinScopeOutOfBinDollarFilesStayRaw]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.RecycleBinScopeOutOfBinDollarFilesStayRaw]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RecycleBinScopeOutOfBinDollarFilesStayRaw]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5232]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.LostDirGroupsOrphansByParentRecord]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.LostDirGroupsOrphansByParentRecord]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.LostDirGroupsOrphansByParentRecord]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5460]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.LostDirNameIsRecordNumber]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.LostDirNameIsRecordNumber]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.LostDirNameIsRecordNumber]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5505]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.TornMftRecord0BootstrapsFromMirror]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.TornMftRecord0BootstrapsFromMirror]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.TornMftRecord0BootstrapsFromMirror]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5542]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.MirrorBootstrapSameRunlistAsIntactControl]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.MirrorBootstrapSameRunlistAsIntactControl]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.MirrorBootstrapSameRunlistAsIntactControl]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5586]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BothMftAndMirrorTornRefuses]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.BothMftAndMirrorTornRefuses]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BothMftAndMirrorTornRefuses]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5634]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.MirrorBootstrapFlaggedInOpenDiagnostics]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NtfsVolume.MirrorBootstrapFlaggedInOpenDiagnostics]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.MirrorBootstrapFlaggedInOpenDiagnostics]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:5650]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.FreeMapExcludesVirtualClustersAndMatchesFAT]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.FreeMapExcludesVirtualClustersAndMatchesFAT]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.FreeMapExcludesVirtualClustersAndMatchesFAT]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:202]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat16EndToEnd]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.Fat16EndToEnd]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat16EndToEnd]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:248]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.LfnIllegalCharFallsBackTo83]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.LfnIllegalCharFallsBackTo83]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.LfnIllegalCharFallsBackTo83]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:309]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.LfnValidNameUnchanged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.LfnValidNameUnchanged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.LfnValidNameUnchanged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:373]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.LfnCorruptFallsBackToShort]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.LfnCorruptFallsBackToShort]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.LfnCorruptFallsBackToShort]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:392]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.ShortNameOemBytesDecoded]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.ShortNameOemBytesDecoded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.ShortNameOemBytesDecoded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:411]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.ShortNameAsciiUnchanged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.ShortNameAsciiUnchanged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.ShortNameAsciiUnchanged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:450]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.ShortNameKaniMarkerPathUnchanged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.ShortNameKaniMarkerPathUnchanged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.ShortNameKaniMarkerPathUnchanged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:473]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.NtResCaseFoldFourWay]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.NtResCaseFoldFourWay]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.NtResCaseFoldFourWay]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:511]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.NtResFoldLeavesLfnCaseUntouched]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.NtResFoldLeavesLfnCaseUntouched]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.NtResFoldLeavesLfnCaseUntouched]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:562]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat32RootChainAndBackupBoot]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.Fat32RootChainAndBackupBoot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat32RootChainAndBackupBoot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:584]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat16HasNoBackupBootFallback]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.Fat16HasNoBackupBootFallback]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat16HasNoBackupBootFallback]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:667]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat32BackupBootFallback]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.Fat32BackupBootFallback]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat32BackupBootFallback]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:683]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.ChainCycleIsCut]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.ChainCycleIsCut]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.ChainCycleIsCut]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:731]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat12ChainsCanonicalEocAndBad]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.Fat12ChainsCanonicalEocAndBad]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat12ChainsCanonicalEocAndBad]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:834]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.FatOriginClusterCarriedForChainlessDeleted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.FatOriginClusterCarriedForChainlessDeleted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.FatOriginClusterCarriedForChainlessDeleted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:877]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.ParentAnchorCarriesContainingDirCluster]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.ParentAnchorCarriesContainingDirCluster]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.ParentAnchorCarriesContainingDirCluster]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:935]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedLfnFragmentOrder]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedLfnFragmentOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedLfnFragmentOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1002]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedLfnRealWorldChecksumRecoversFullName]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedLfnRealWorldChecksumRecoversFullName]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedLfnRealWorldChecksumRecoversFullName]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1052]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedLfnSingleSlotChecksumRecovery]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedLfnSingleSlotChecksumRecovery]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedLfnSingleSlotChecksumRecovery]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1089]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedLfnTornChecksumKeepsHonestFallback]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedLfnTornChecksumKeepsHonestFallback]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedLfnTornChecksumKeepsHonestFallback]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1119]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.LfnDeletedChecksumBijection]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.LfnDeletedChecksumBijection]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.LfnDeletedChecksumBijection]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1154]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedUnlinkedChainGetsContiguousRun]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedUnlinkedChainGetsContiguousRun]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedUnlinkedChainGetsContiguousRun]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1190]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.TruncatedVolumeOverflowChainStance]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.TruncatedVolumeOverflowChainStance]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.TruncatedVolumeOverflowChainStance]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1224]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedUnlinkedChainTruncatesAtReusedCluster]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedUnlinkedChainTruncatesAtReusedCluster]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedUnlinkedChainTruncatesAtReusedCluster]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1286]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedLinkedChainStillPrefersFatTruth]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedLinkedChainStillPrefersFatTruth]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedLinkedChainStillPrefersFatTruth]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1317]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat32EocMask24AndAlternateFamiliesEndClean]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.Fat32EocMask24AndAlternateFamiliesEndClean]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat32EocMask24AndAlternateFamiliesEndClean]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1344]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat16BadClusterStillCanonical]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.Fat16BadClusterStillCanonical]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat16BadClusterStillCanonical]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1408]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.LargeFat32TableAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.LargeFat32TableAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.LargeFat32TableAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1483]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.SmallFat16TableDeclined]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.SmallFat16TableDeclined]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.SmallFat16TableDeclined]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1531]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.MirrorShiftedCopyRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.MirrorShiftedCopyRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.MirrorShiftedCopyRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1576]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.MirrorCrossLinkRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.MirrorCrossLinkRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.MirrorCrossLinkRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1627]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.MirrorOnlyScanFat1Unreadable]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.MirrorOnlyScanFat1Unreadable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.MirrorOnlyScanFat1Unreadable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1657]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.CrossLinkedFat1RejectedAndFat2Used]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.CrossLinkedFat1RejectedAndFat2Used]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.CrossLinkedFat1RejectedAndFat2Used]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1705]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.MediaByteCanonicalEnforcedFat16]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.MediaByteCanonicalEnforcedFat16]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.MediaByteCanonicalEnforcedFat16]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1740]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.InvalidDateYieldsZeroFileTime]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.InvalidDateYieldsZeroFileTime]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.InvalidDateYieldsZeroFileTime]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1777]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.InvalidMonthYieldsZeroFileTime]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.InvalidMonthYieldsZeroFileTime]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.InvalidMonthYieldsZeroFileTime]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1798]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.InvalidDayYieldsZeroFileTime]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.InvalidDayYieldsZeroFileTime]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.InvalidDayYieldsZeroFileTime]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1815]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.ValidDateYieldsCorrectFileTime]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.ValidDateYieldsCorrectFileTime]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.ValidDateYieldsCorrectFileTime]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1832]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.TimeWordHour24YieldsZeroFileTime]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.TimeWordHour24YieldsZeroFileTime]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.TimeWordHour24YieldsZeroFileTime]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1869]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.TimeWordMinute60YieldsZeroFileTime]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.TimeWordMinute60YieldsZeroFileTime]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.TimeWordMinute60YieldsZeroFileTime]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1877]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.TimeWordSecondField30YieldsZeroFileTime]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.TimeWordSecondField30YieldsZeroFileTime]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.TimeWordSecondField30YieldsZeroFileTime]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1882]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.ValidTimeWordBoundaryUnchanged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.ValidTimeWordBoundaryUnchanged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.ValidTimeWordBoundaryUnchanged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1889]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.InvalidTimeStampsPersistAsZeroInTree]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.InvalidTimeStampsPersistAsZeroInTree]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.InvalidTimeStampsPersistAsZeroInTree]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1897]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.FlavorBoundary65524IsFat32]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.FlavorBoundary65524IsFat32]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.FlavorBoundary65524IsFat32]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1920]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.BootStringGateDeclinesTornStringVolume]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.BootStringGateDeclinesTornStringVolume]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.BootStringGateDeclinesTornStringVolume]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1931]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.BootStringCaseInsensitiveAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.BootStringCaseInsensitiveAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.BootStringCaseInsensitiveAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1947]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat16LowRootEntriesDeclined]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.Fat16LowRootEntriesDeclined]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat16LowRootEntriesDeclined]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1961]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat16RootEntriesNotSectorMultipleDeclined]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.Fat16RootEntriesNotSectorMultipleDeclined]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat16RootEntriesNotSectorMultipleDeclined]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1975]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.NumFatsThreeDeclined]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.NumFatsThreeDeclined]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.NumFatsThreeDeclined]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1989]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.SpcDomainMatchesOgSet]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.SpcDomainMatchesOgSet]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.SpcDomainMatchesOgSet]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2003]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.FatBps1536Accepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.FatBps1536Accepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.FatBps1536Accepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2031]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.FatBps32768CeilingAndNonMultipleRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.FatBps32768CeilingAndNonMultipleRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.FatBps32768CeilingAndNonMultipleRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2049]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.FatZeroNumFatsRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.FatZeroNumFatsRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.FatZeroNumFatsRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2070]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.FatClusterCountOverflowRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.FatClusterCountOverflowRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.FatClusterCountOverflowRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2084]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DateSanityRejectedInStrictMode]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DateSanityRejectedInStrictMode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DateSanityRejectedInStrictMode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2103]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.BadPrimaryServedWithoutMirrorConsult]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.BadPrimaryServedWithoutMirrorConsult]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.BadPrimaryServedWithoutMirrorConsult]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2138]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.PrimaryFreeStillConsultsMirror]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.PrimaryFreeStillConsultsMirror]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.PrimaryFreeStillConsultsMirror]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2168]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.RecyclePairingRestoresOriginalName]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.RecyclePairingRestoresOriginalName]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.RecyclePairingRestoresOriginalName]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2267]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.RecycleLegacyRoots]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.RecycleLegacyRoots]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.RecycleLegacyRoots]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2300]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.NonRecycleDollarFilesStayRaw]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.NonRecycleDollarFilesStayRaw]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.NonRecycleDollarFilesStayRaw]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2318]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.ReadVolumeLabelDecodesBootSector]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.ReadVolumeLabelDecodesBootSector]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.ReadVolumeLabelDecodesBootSector]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2382]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.ReadVolumeLabelRejectsLabelLessAndNonFat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.ReadVolumeLabelRejectsLabelLessAndNonFat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.ReadVolumeLabelRejectsLabelLessAndNonFat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2398]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.ClusterOffsetBeyond2TiB]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.ClusterOffsetBeyond2TiB]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.ClusterOffsetBeyond2TiB]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2421]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedFirstCharPlaceholderIsHash]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedFirstCharPlaceholderIsHash]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedFirstCharPlaceholderIsHash]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2449]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedLfnChecksumRecoversOriginalByte]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedLfnChecksumRecoversOriginalByte]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedLfnChecksumRecoversOriginalByte]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2463]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Date2107ConvertsFullyNoClamp]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.Date2107ConvertsFullyNoClamp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Date2107ConvertsFullyNoClamp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2489]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.StringFat16LowClusterCountScansAsFat16]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.StringFat16LowClusterCountScansAsFat16]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.StringFat16LowClusterCountScansAsFat16]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2513]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.StringFat32UnderBracketScansAsFat32]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.StringFat32UnderBracketScansAsFat32]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.StringFat32UnderBracketScansAsFat32]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2568]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.HybridStringFat16MediaFat32ReflavorsToFat32]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.HybridStringFat16MediaFat32ReflavorsToFat32]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.HybridStringFat16MediaFat32ReflavorsToFat32]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2633]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat2ReadWinsOverFat1]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.Fat2ReadWinsOverFat1]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat2ReadWinsOverFat1]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2694]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.ConsistentVolumeUnreflavored]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.ConsistentVolumeUnreflavored]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.ConsistentVolumeUnreflavored]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2721]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.StringlessSmallVolumeStillBracketsFat12]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.StringlessSmallVolumeStillBracketsFat12]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.StringlessSmallVolumeStillBracketsFat12]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2741]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.StringGateRejectsShapelessVolume]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.StringGateRejectsShapelessVolume]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.StringGateRejectsShapelessVolume]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2789]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.TimeWordOutOfRangeYieldsZeroTimestamp]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.TimeWordOutOfRangeYieldsZeroTimestamp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.TimeWordOutOfRangeYieldsZeroTimestamp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2808]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.ValidTimeWordBoundaryConvertsUnchanged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.ValidTimeWordBoundaryConvertsUnchanged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.ValidTimeWordBoundaryConvertsUnchanged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2833]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.InvalidTimeStampsPersistAsZero]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.InvalidTimeStampsPersistAsZero]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.InvalidTimeStampsPersistAsZero]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2845]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.InvalidTimeWordAccessSlotStaysReal]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.InvalidTimeWordAccessSlotStaysReal]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.InvalidTimeWordAccessSlotStaysReal]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2888]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DentryAttrReservedBitsStopWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DentryAttrReservedBitsStopWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DentryAttrReservedBitsStopWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2942]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DentryErasedPatternQuadStopsWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DentryErasedPatternQuadStopsWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DentryErasedPatternQuadStopsWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2967]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DentryHistogramEleventhRepeatStopsWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DentryHistogramEleventhRepeatStopsWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DentryHistogramEleventhRepeatStopsWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:2994]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DentryHistogramTenRepeatsStillEmit]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DentryHistogramTenRepeatsStillEmit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DentryHistogramTenRepeatsStillEmit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:3025]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DentryImpureLabelStopsWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DentryImpureLabelStopsWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DentryImpureLabelStopsWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:3049]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.PureVolumeLabelSkippedWalkContinues]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.PureVolumeLabelSkippedWalkContinues]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.PureVolumeLabelSkippedWalkContinues]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:3071]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.SizeChainXorSkippedButWalkContinues]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.SizeChainXorSkippedButWalkContinues]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.SizeChainXorSkippedButWalkContinues]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:3090]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DirSizeAndHighClusterSkipped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.DirSizeAndHighClusterSkipped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DirSizeAndHighClusterSkipped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:3114]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Date2107EndToEndFileTimeIsReal]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.Date2107EndToEndFileTimeIsReal]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Date2107EndToEndFileTimeIsReal]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:3134]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.AccessSlotStampsDateAtMidnightWhenTimeWordInvalid]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatEngine.AccessSlotStampsDateAtMidnightWhenTimeWordInvalid]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.AccessSlotStampsDateAtMidnightWhenTimeWordInvalid]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:3160]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatChain.RunCoalescingDetectsOverlap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatChain.RunCoalescingDetectsOverlap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatChain.RunCoalescingDetectsOverlap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:3202]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatChain.CycleGuardPerf]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FatChain.CycleGuardPerf]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatChain.CycleGuardPerf]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:3267]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.LostAnchorBackupBootRebasedToTrueStart]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.LostAnchorBackupBootRebasedToTrueStart]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.LostAnchorBackupBootRebasedToTrueStart]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:93]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.HardeningBoundsPinned]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.HardeningBoundsPinned]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.HardeningBoundsPinned]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:142]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.IntactPrimaryNotShifted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.IntactPrimaryNotShifted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.IntactPrimaryNotShifted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:229]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.MediaWordsAbsentNoShift]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.MediaWordsAbsentNoShift]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.MediaWordsAbsentNoShift]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:244]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.ShiftedBaseBelowZeroRefused]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.ShiftedBaseBelowZeroRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.ShiftedBaseBelowZeroRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:263]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.TimestampsMatchOgLocalAsUtc]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.TimestampsMatchOgLocalAsUtc]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.TimestampsMatchOgLocalAsUtc]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:309]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.TimestampsZeroTzPassThrough]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.TimestampsZeroTzPassThrough]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.TimestampsZeroTzPassThrough]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:330]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.TimestampsMatchExplorerDisplay]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.TimestampsMatchExplorerDisplay]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.TimestampsMatchExplorerDisplay]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:344]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.TimestampsWestOffsetAlsoIgnored]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.TimestampsWestOffsetAlsoIgnored]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.TimestampsWestOffsetAlsoIgnored]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:360]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.OriginClusterCarriedFromStreamDentry]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.OriginClusterCarriedFromStreamDentry]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.OriginClusterCarriedFromStreamDentry]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:375]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.FreeMapExcludesVirtualClustersAndMatchesBitmap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.FreeMapExcludesVirtualClustersAndMatchesBitmap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.FreeMapExcludesVirtualClustersAndMatchesBitmap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:427]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.TornBitmapReadYieldsNoFreeMap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.TornBitmapReadYieldsNoFreeMap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.TornBitmapReadYieldsNoFreeMap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:498]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.EndToEnd]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.EndToEnd]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.EndToEnd]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:521]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.UpcaseTableChecksumValidated]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.UpcaseTableChecksumValidated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.UpcaseTableChecksumValidated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:608]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.UpcaseLengthSanity]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.UpcaseLengthSanity]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.UpcaseLengthSanity]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:649]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.FreeMapProducerMatchesSpecBitMapping]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.FreeMapProducerMatchesSpecBitMapping]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.FreeMapProducerMatchesSpecBitMapping]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:690]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.SetChecksumMismatchGradesExcellentInOgProfile]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.SetChecksumMismatchGradesExcellentInOgProfile]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.SetChecksumMismatchGradesExcellentInOgProfile]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:753]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.SetChecksumMismatchMarksDamaged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.SetChecksumMismatchMarksDamaged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.SetChecksumMismatchMarksDamaged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:774]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.InvalidFirstClusterRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.InvalidFirstClusterRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.InvalidFirstClusterRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:801]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.EocFamilyTerminatesChain]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.EocFamilyTerminatesChain]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.EocFamilyTerminatesChain]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:847]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.FreeEntryContiguousSalvage]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.FreeEntryContiguousSalvage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.FreeEntryContiguousSalvage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:881]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.BootGatesMatchOg]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.BootGatesMatchOg]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.BootGatesMatchOg]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:917]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.UpcaseAlgorithmMatchesRealMkfsExfat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.UpcaseAlgorithmMatchesRealMkfsExfat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.UpcaseAlgorithmMatchesRealMkfsExfat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1132]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.LargeFatFullChains]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.LargeFatFullChains]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.LargeFatFullChains]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1151]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.LargeFatDirectoryWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.LargeFatDirectoryWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.LargeFatDirectoryWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1191]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.StreamReservedPartiallyNonzeroPassesOgByteOr]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.StreamReservedPartiallyNonzeroPassesOgByteOr]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.StreamReservedPartiallyNonzeroPassesOgByteOr]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1222]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.StreamReservedAllBytesNonzeroStillDamaged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.StreamReservedAllBytesNonzeroStillDamaged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.StreamReservedAllBytesNonzeroStillDamaged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1261]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.StreamReserved1ByteStrictlyZero]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.StreamReserved1ByteStrictlyZero]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.StreamReserved1ByteStrictlyZero]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1287]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.DeletedSetChecksumRejectsTornSet]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.DeletedSetChecksumRejectsTornSet]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.DeletedSetChecksumRejectsTornSet]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1314]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.DeletedSetOracleReassembly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.DeletedSetOracleReassembly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.DeletedSetOracleReassembly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1343]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.LiveCompleteSetBadChecksumUngraded]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.LiveCompleteSetBadChecksumUngraded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.LiveCompleteSetBadChecksumUngraded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1389]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.AppleDoublePrefixSunk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.AppleDoublePrefixSunk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.AppleDoublePrefixSunk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1441]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.RegularDotFilesNotSunk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.RegularDotFilesNotSunk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.RegularDotFilesNotSunk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1482]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.EmptyFileSuppressedLikeOg]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.EmptyFileSuppressedLikeOg]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.EmptyFileSuppressedLikeOg]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1518]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.GarbageTimestampRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.GarbageTimestampRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.GarbageTimestampRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1553]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.SecondaryCountCapped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.SecondaryCountCapped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.SecondaryCountCapped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1606]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.BadNameFlagsRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.BadNameFlagsRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.BadNameFlagsRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1642]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.FileReservedNonzeroRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.FileReservedNonzeroRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.FileReservedNonzeroRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1691]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.ValidBoundaryTimestampsStillPass]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.ValidBoundaryTimestampsStillPass]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.ValidBoundaryTimestampsStillPass]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1727]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.NameUtf16TableInvalidCharRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.NameUtf16TableInvalidCharRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.NameUtf16TableInvalidCharRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1785]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.NameUtf16TableAllowedCharAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.NameUtf16TableAllowedCharAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.NameUtf16TableAllowedCharAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1807]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.RecyclePairingMirrored]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExfatEngine.RecyclePairingMirrored]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.RecyclePairingMirrored]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1832]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.ExtentTreeDepth3]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.ExtentTreeDepth3]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.ExtentTreeDepth3]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:275]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.SuperblockValidation]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.SuperblockValidation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.SuperblockValidation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:412]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.SbBandsPinned]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.SbBandsPinned]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.SbBandsPinned]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:446]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.EndToEndWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.EndToEndWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.EndToEndWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:523]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.BadMagicRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.BadMagicRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.BadMagicRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:576]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.UnixTimeConversion]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.UnixTimeConversion]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.UnixTimeConversion]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:589]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.SizeExceedsBlocksDamagedDirectoryStaysUnflagged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.SizeExceedsBlocksDamagedDirectoryStaysUnflagged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.SizeExceedsBlocksDamagedDirectoryStaysUnflagged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:614]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.LegacyAclDirSizeClean]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.LegacyAclDirSizeClean]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.LegacyAclDirSizeClean]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:675]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.Post2038FileTimeConversionStays64Bit]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.Post2038FileTimeConversionStays64Bit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.Post2038FileTimeConversionStays64Bit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:708]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.SizeHighFoldIsRegularOnlyNotTheOgUnconditionalDefect]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.SizeHighFoldIsRegularOnlyNotTheOgUnconditionalDefect]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.SizeHighFoldIsRegularOnlyNotTheOgUnconditionalDefect]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:718]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.UnwrittenExtentLenDecodedKernelCorrect]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.UnwrittenExtentLenDecodedKernelCorrect]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.UnwrittenExtentLenDecodedKernelCorrect]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:765]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.SizeExceedsBlocksSetsDamagedFlagConvergent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.SizeExceedsBlocksSetsDamagedFlagConvergent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.SizeExceedsBlocksSetsDamagedFlagConvergent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:842]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.DamagedFlagCleanFilesCarryNoBit]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.DamagedFlagCleanFilesCarryNoBit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.DamagedFlagCleanFilesCarryNoBit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:902]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.DamagedFlagSessionRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.DamagedFlagSessionRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.DamagedFlagSessionRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:929]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.JournalSalvage]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.JournalSalvage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.JournalSalvage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:968]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.InodeSizeOffsetIsSpecTrue]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.InodeSizeOffsetIsSpecTrue]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.InodeSizeOffsetIsSpecTrue]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:997]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.JournalUncommittedTailSurfaces]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.JournalUncommittedTailSurfaces]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.JournalUncommittedTailSurfaces]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1013]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.FamilyKindStampsPerDecodedAnalyserRule]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.FamilyKindStampsPerDecodedAnalyserRule]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.FamilyKindStampsPerDecodedAnalyserRule]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1054]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.JournalUnreferencedDirBlockSalvaged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.JournalUnreferencedDirBlockSalvaged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.JournalUnreferencedDirBlockSalvaged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1118]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.JournalFirstBlockNotJsbNoSalvage]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.JournalFirstBlockNotJsbNoSalvage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.JournalFirstBlockNotJsbNoSalvage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1152]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.CorruptRecLenSkipsEntryNotBlock]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.CorruptRecLenSkipsEntryNotBlock]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.CorruptRecLenSkipsEntryNotBlock]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1185]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.InodeZeroHoleContinues]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.InodeZeroHoleContinues]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.InodeZeroHoleContinues]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1222]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.FileTypeGateSkipsInvalid]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.FileTypeGateSkipsInvalid]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.FileTypeGateSkipsInvalid]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1265]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.NonAsciiNamesDecodedNotByteLifted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.NonAsciiNamesDecodedNotByteLifted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.NonAsciiNamesDecodedNotByteLifted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1308]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.MetaBgVolumeRecoversAllGroups]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.MetaBgVolumeRecoversAllGroups]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.MetaBgVolumeRecoversAllGroups]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1536]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.MetaBgDeletedSweepReachesDistributedGroups]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.MetaBgDeletedSweepReachesDistributedGroups]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.MetaBgDeletedSweepReachesDistributedGroups]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1558]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.MetaBgDescriptorBlockReadOnce]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.MetaBgDescriptorBlockReadOnce]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.MetaBgDescriptorBlockReadOnce]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1581]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.MetaBgFirstMetaBgFrontRegion]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.MetaBgFirstMetaBgFrontRegion]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.MetaBgFirstMetaBgFrontRegion]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1602]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.MetaBgSuperblockFieldsParsed]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.MetaBgSuperblockFieldsParsed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.MetaBgSuperblockFieldsParsed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1622]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.BackupSuperblockRescueWhenPrimaryDestroyed]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.BackupSuperblockRescueWhenPrimaryDestroyed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.BackupSuperblockRescueWhenPrimaryDestroyed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1715]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.BackupRescueFallsBackToBackupGdt]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.BackupRescueFallsBackToBackupGdt]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.BackupRescueFallsBackToBackupGdt]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1738]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.BackupRescueWholeDiskRebase]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.BackupRescueWholeDiskRebase]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.BackupRescueWholeDiskRebase]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1756]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.BadMagicWithoutBackupsStillRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.BadMagicWithoutBackupsStillRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.BadMagicWithoutBackupsStillRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1790]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.JournalSalvagedDirReparented]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.JournalSalvagedDirReparented]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.JournalSalvagedDirReparented]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1825]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.JournalUnreferencedOrphanFlatWithRealName]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.JournalUnreferencedOrphanFlatWithRealName]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.JournalUnreferencedOrphanFlatWithRealName]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1884]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.TwoPassJournalClaimInteraction]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.TwoPassJournalClaimInteraction]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.TwoPassJournalClaimInteraction]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1917]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.CatchBufferBacktrackSalvage]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.CatchBufferBacktrackSalvage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.CatchBufferBacktrackSalvage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:2003]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.BacktrackOffZeroFillsTheWindow]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.BacktrackOffZeroFillsTheWindow]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.BacktrackOffZeroFillsTheWindow]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:2041]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.ZeroFillOnlyWhenBacktrackAlsoFails]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.ZeroFillOnlyWhenBacktrackAlsoFails]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.ZeroFillOnlyWhenBacktrackAlsoFails]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:2080]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.NanosecondsPreservedFromExtraField]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.NanosecondsPreservedFromExtraField]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.NanosecondsPreservedFromExtraField]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:2148]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.InvalidUtf8NameCarriesRawBytes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.InvalidUtf8NameCarriesRawBytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.InvalidUtf8NameCarriesRawBytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:2228]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.PoisonDoubleIndirectBoundedBySize]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.PoisonDoubleIndirectBoundedBySize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.PoisonDoubleIndirectBoundedBySize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:2344]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.PoisonTripleIndirectBoundedBySize]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.PoisonTripleIndirectBoundedBySize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.PoisonTripleIndirectBoundedBySize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:2378]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.StaleIndirectCancelResponsive]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.StaleIndirectCancelResponsive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.StaleIndirectCancelResponsive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:2409]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.ContiguousPhysicalBlocksMergedToSingleExtent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4Engine.ContiguousPhysicalBlocksMergedToSingleExtent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.ContiguousPhysicalBlocksMergedToSingleExtent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:2434]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4EngineM144.Large64BitVolumeAbove8MiBGdtOpens]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4EngineM144.Large64BitVolumeAbove8MiBGdtOpens]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4EngineM144.Large64BitVolumeAbove8MiBGdtOpens]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_m144_large_volume.cpp:105]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4EngineM144.CorruptHugeGroupCountStillRefused]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4EngineM144.CorruptHugeGroupCountStillRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4EngineM144.CorruptHugeGroupCountStillRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_m144_large_volume.cpp:122]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4EngineM143.JournalInodeImageSurfacesInodeRecord]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4EngineM143.JournalInodeImageSurfacesInodeRecord]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4EngineM143.JournalInodeImageSurfacesInodeRecord]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_m143_journal_inode_image.cpp:267]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4EngineM143.JournalInodeImageZeroSizeSkipped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4EngineM143.JournalInodeImageZeroSizeSkipped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4EngineM143.JournalInodeImageZeroSizeSkipped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_m143_journal_inode_image.cpp:281]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4EngineM143.JournalInodeImageNoExtentsFlagSkipped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4EngineM143.JournalInodeImageNoExtentsFlagSkipped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4EngineM143.JournalInodeImageNoExtentsFlagSkipped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_m143_journal_inode_image.cpp:290]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4EngineM143.JournalInodeImageBadExtentMagicSkipped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4EngineM143.JournalInodeImageBadExtentMagicSkipped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4EngineM143.JournalInodeImageBadExtentMagicSkipped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_m143_journal_inode_image.cpp:299]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4EngineM143.JournalInodeImageZeroEhEntriesSkipped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4EngineM143.JournalInodeImageZeroEhEntriesSkipped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4EngineM143.JournalInodeImageZeroEhEntriesSkipped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_m143_journal_inode_image.cpp:308]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4EngineM143.JournalDirPairTakesPrecedence]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ext4EngineM143.JournalDirPairTakesPrecedence]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4EngineM143.JournalDirPairTakesPrecedence]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_m143_journal_inode_image.cpp:324]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.VolumeHeaderValidation]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.VolumeHeaderValidation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.VolumeHeaderValidation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:209]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.VhFreeBlocksAboveTotalRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.VhFreeBlocksAboveTotalRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.VhFreeBlocksAboveTotalRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:246]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.VhJournalInfoBlockAboveTotalRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.VhJournalInfoBlockAboveTotalRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.VhJournalInfoBlockAboveTotalRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:263]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.VhTornAllocForkStillValidates]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.VhTornAllocForkStillValidates]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.VhTornAllocForkStillValidates]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:279]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.VhFreeBlocksEqualTotalAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.VhFreeBlocksEqualTotalAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.VhFreeBlocksEqualTotalAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:298]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.EndToEndWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.EndToEndWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.EndToEndWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:314]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.DecmpfsInlineAndRsrcContentRecovered]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.DecmpfsInlineAndRsrcContentRecovered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.DecmpfsInlineAndRsrcContentRecovered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:438]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.DecmpfsDisabledLeavesEmptyShell]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.DecmpfsDisabledLeavesEmptyShell]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.DecmpfsDisabledLeavesEmptyShell]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:572]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.MacTimeConversion]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.MacTimeConversion]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.MacTimeConversion]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:619]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.BadSignatureRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.BadSignatureRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.BadSignatureRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:626]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.LeafNodeLayoutMatchesOnDiskFormat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.LeafNodeLayoutMatchesOnDiskFormat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.LeafNodeLayoutMatchesOnDiskFormat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:644]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.DotfileAndPrivateDataFiltered]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.DotfileAndPrivateDataFiltered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.DotfileAndPrivateDataFiltered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:679]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.PrdfRecordFolderized]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.PrdfRecordFolderized]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.PrdfRecordFolderized]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:733]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.RecordBit29SelectsResourceFork]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.RecordBit29SelectsResourceFork]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.RecordBit29SelectsResourceFork]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:781]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.LargeBlockSizeAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.LargeBlockSizeAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.LargeBlockSizeAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:848]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.ExtentsOverflowRecordResolvesFragmentedFork]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.ExtentsOverflowRecordResolvesFragmentedFork]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.ExtentsOverflowRecordResolvesFragmentedFork]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:948]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalHarvestEmitsDeletedRecords]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalHarvestEmitsDeletedRecords]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalHarvestEmitsDeletedRecords]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1089]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalDifferentialDifferentFileIdKept]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalDifferentialDifferentFileIdKept]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalDifferentialDifferentFileIdKept]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1137]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalBadHeaderGates]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalBadHeaderGates]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalBadHeaderGates]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1167]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalWrapHandled]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalWrapHandled]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalWrapHandled]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1199]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalHarvestDisabledByFlag]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalHarvestDisabledByFlag]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalHarvestDisabledByFlag]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1246]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalSameKeyDifferentFileIdBothKept]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalSameKeyDifferentFileIdBothKept]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalSameKeyDifferentFileIdBothKept]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1265]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalAttrDecmpfsFallbackFeedsEmptyForkFile]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalAttrDecmpfsFallbackFeedsEmptyForkFile]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalAttrDecmpfsFallbackFeedsEmptyForkFile]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1302]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalRequiresVolumeJournaledBit]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalRequiresVolumeJournaledBit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalRequiresVolumeJournaledBit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1370]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalInfoBlockHasNoMagicField]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalInfoBlockHasNoMagicField]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalInfoBlockHasNoMagicField]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1390]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalNodeImageRejectsInconsistentOffsetTable]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalNodeImageRejectsInconsistentOffsetTable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalNodeImageRejectsInconsistentOffsetTable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1412]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalFallbackSalvageWhenHeaderInvalid]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalFallbackSalvageWhenHeaderInvalid]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalFallbackSalvageWhenHeaderInvalid]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1437]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalFallbackRequiresNodeShapedStart]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalFallbackRequiresNodeShapedStart]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalFallbackRequiresNodeShapedStart]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1473]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.ZeroLogicalSizeFallsBackToTotalBlocks]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=HfsPlus.ZeroLogicalSizeFallsBackToTotalBlocks]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.ZeroLogicalSizeFallsBackToTotalBlocks]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1494]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.MtimeNsecFoldsLikeTheOg]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.MtimeNsecFoldsLikeTheOg]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.MtimeNsecFoldsLikeTheOg]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:379]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.SuperblockValidation]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.SuperblockValidation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.SuperblockValidation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:413]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.NonPow2NodesizeRejectedRegistered]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.NonPow2NodesizeRejectedRegistered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.NonPow2NodesizeRejectedRegistered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:450]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.EndToEndWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.EndToEndWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.EndToEndWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:469]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.TwoSubvolumesRootSeparately]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.TwoSubvolumesRootSeparately]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.TwoSubvolumesRootSeparately]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:529]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.SubvolumeNamedFromRootRef]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.SubvolumeNamedFromRootRef]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.SubvolumeNamedFromRootRef]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:702]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.FileAttributeNormalBit]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.FileAttributeNormalBit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.FileAttributeNormalBit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:786]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.BadMagicRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.BadMagicRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.BadMagicRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:815]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.PackedDirItemsInOneItem]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.PackedDirItemsInOneItem]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.PackedDirItemsInOneItem]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:825]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.MirrorSelectionNewestGeneration]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.MirrorSelectionNewestGeneration]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.MirrorSelectionNewestGeneration]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:878]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.InternalKeyPtrWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.InternalKeyPtrWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.InternalKeyPtrWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:912]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.InlineAndCompressedExtents]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.InlineAndCompressedExtents]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.InlineAndCompressedExtents]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1022]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.NonAsciiNamesDecodedNotByteLifted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.NonAsciiNamesDecodedNotByteLifted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.NonAsciiNamesDecodedNotByteLifted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1168]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.Latin1NameRoundTripsRawBytes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.Latin1NameRoundTripsRawBytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.Latin1NameRoundTripsRawBytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1238]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.XattrDosAttribApplied]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.XattrDosAttribApplied]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.XattrDosAttribApplied]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1325]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.UserXattrRecordsEmitted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.UserXattrRecordsEmitted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.UserXattrRecordsEmitted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1393]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.CompressionPropertyTagged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.CompressionPropertyTagged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.CompressionPropertyTagged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1468]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.Raid10GeometryGate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.Raid10GeometryGate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.Raid10GeometryGate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1559]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.Raid1RotatingMirrorReads]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.Raid1RotatingMirrorReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.Raid1RotatingMirrorReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1746]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.Raid0StripeMathSplitsAtStripeBoundaries]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.Raid0StripeMathSplitsAtStripeBoundaries]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.Raid0StripeMathSplitsAtStripeBoundaries]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1776]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.DefaultDirItemNonSubvolumeLocationFallsBack]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.DefaultDirItemNonSubvolumeLocationFallsBack]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.DefaultDirItemNonSubvolumeLocationFallsBack]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1897]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.CacheRoundTripAndInvalidation]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.CacheRoundTripAndInvalidation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.CacheRoundTripAndInvalidation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1980]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.CacheDisabledByDefault]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.CacheDisabledByDefault]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.CacheDisabledByDefault]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:2045]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.CacheWarmScanSkipsChunkDerivation]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.CacheWarmScanSkipsChunkDerivation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.CacheWarmScanSkipsChunkDerivation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:2080]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.CacheStaleGenerationDiscardedOnVolumeRewrite]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.CacheStaleGenerationDiscardedOnVolumeRewrite]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.CacheStaleGenerationDiscardedOnVolumeRewrite]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:2132]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.EaDirSubtreeSuppressed]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.EaDirSubtreeSuppressed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.EaDirSubtreeSuppressed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:2187]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.EaDirExactMatchOnly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.EaDirExactMatchOnly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.EaDirExactMatchOnly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:2208]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.EaDirNodeItselfEmitted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.EaDirNodeItselfEmitted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.EaDirNodeItselfEmitted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:2226]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.EaDirNotInFilterfolderPools]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Btrfs.EaDirNotInFilterfolderPools]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.EaDirNotInFilterfolderPools]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:2246]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.EndToEndWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.EndToEndWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.EndToEndWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:903]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.ChildDirZapReadsKernelOffset32]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.ChildDirZapReadsKernelOffset32]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.ChildDirZapReadsKernelOffset32]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:984]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.OriginObjNotTreatedAsChildZap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.OriginObjNotTreatedAsChildZap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.OriginObjNotTreatedAsChildZap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1008]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.LabelPositionsRecoveredFromTail]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.LabelPositionsRecoveredFromTail]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.LabelPositionsRecoveredFromTail]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1046]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.NewestTxgAcrossLabelsWins]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.NewestTxgAcrossLabelsWins]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.NewestTxgAcrossLabelsWins]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1060]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.RingWindowDeletedFileRecalled]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.RingWindowDeletedFileRecalled]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.RingWindowDeletedFileRecalled]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1192]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.RingWindowUnchangedEntriesEmitOnce]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.RingWindowUnchangedEntriesEmitOnce]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.RingWindowUnchangedEntriesEmitOnce]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1226]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.RingWindowModifiedFilePriorVersionNotReemitted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.RingWindowModifiedFilePriorVersionNotReemitted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.RingWindowModifiedFilePriorVersionNotReemitted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1261]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.RingTornNewestStateFallsBackToOlder]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.RingTornNewestStateFallsBackToOlder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.RingTornNewestStateFallsBackToOlder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1294]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.ChecksumFailDropsAndRetriesNextDva]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.ChecksumFailDropsAndRetriesNextDva]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.ChecksumFailDropsAndRetriesNextDva]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1339]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.StripedPoolForeignVdevFallbackRead]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.StripedPoolForeignVdevFallbackRead]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.StripedPoolForeignVdevFallbackRead]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1394]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.MirrorSecondMemberScan]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.MirrorSecondMemberScan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.MirrorSecondMemberScan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1464]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.SaRegistryDrivenLayout2]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.SaRegistryDrivenLayout2]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.SaRegistryDrivenLayout2]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1499]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.SaRegistryAbsentFallsBackToLayout1]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.SaRegistryAbsentFallsBackToLayout1]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.SaRegistryAbsentFallsBackToLayout1]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1543]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.BadMagicRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.BadMagicRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.BadMagicRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1583]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.ZleRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.ZleRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.ZleRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1593]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.CodecMapping]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.CodecMapping]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.CodecMapping]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1607]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.CodecRoundTrips]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.CodecRoundTrips]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.CodecRoundTrips]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1688]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.TruncatedSourcesRejectedNotOverread]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.TruncatedSourcesRejectedNotOverread]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.TruncatedSourcesRejectedNotOverread]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1716]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.NonAsciiNamesDecodedNotByteLifted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.NonAsciiNamesDecodedNotByteLifted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.NonAsciiNamesDecodedNotByteLifted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1745]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.LabelRetryBackoff]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.LabelRetryBackoff]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.LabelRetryBackoff]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1850]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.LabelRetryGivesUpBounded]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.LabelRetryGivesUpBounded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.LabelRetryGivesUpBounded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1869]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.StaticMediaNeverRetries]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.StaticMediaNeverRetries]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.StaticMediaNeverRetries]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1883]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.RingWindowRenamedEntryRecalledUnderBothNames]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.RingWindowRenamedEntryRecalledUnderBothNames]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.RingWindowRenamedEntryRecalledUnderBothNames]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1903]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.TrashInfoSidecarChildrenSuppressed]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Zfs.TrashInfoSidecarChildrenSuppressed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.TrashInfoSidecarChildrenSuppressed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1959]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RefsDetect.FindsMsbPages]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RefsDetect.FindsMsbPages]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RefsDetect.FindsMsbPages]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_ldm.cpp:35]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RefsDetect.CheckpointVariant]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RefsDetect.CheckpointVariant]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RefsDetect.CheckpointVariant]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_ldm.cpp:46]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RefsDetect.IdentificationFallback]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RefsDetect.IdentificationFallback]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RefsDetect.IdentificationFallback]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_ldm.cpp:57]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RefsDetect.PartialSignaturePageDetectedAndFlagged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RefsDetect.PartialSignaturePageDetectedAndFlagged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RefsDetect.PartialSignaturePageDetectedAndFlagged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_ldm.cpp:71]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RefsDetect.FullSignaturePageNotFlaggedPartial]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RefsDetect.FullSignaturePageNotFlaggedPartial]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RefsDetect.FullSignaturePageNotFlaggedPartial]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_ldm.cpp:91]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RefsDetect.NonRefsRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RefsDetect.NonRefsRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RefsDetect.NonRefsRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_ldm.cpp:105]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RefsDetect.PartialSignaturePageDetected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RefsDetect.PartialSignaturePageDetected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RefsDetect.PartialSignaturePageDetected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1791]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.ParsesDatabaseAndGraph]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.ParsesDatabaseAndGraph]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.ParsesDatabaseAndGraph]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:58]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.ResolvesSimpleVolume]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.ResolvesSimpleVolume]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.ResolvesSimpleVolume]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:94]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.SimpleVolumeDeviceReads]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.SimpleVolumeDeviceReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.SimpleVolumeDeviceReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:114]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.StripedVolumeInterleave]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.StripedVolumeInterleave]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.StripedVolumeInterleave]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:143]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.CompositeVblkRecord]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.CompositeVblkRecord]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.CompositeVblkRecord]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:178]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.NonLdmDiskRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.NonLdmDiskRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.NonLdmDiskRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:196]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.Raid5LeftSymmetricReads]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.Raid5LeftSymmetricReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.Raid5LeftSymmetricReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:266]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.Raid5ReconstructsMissingColumn]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.Raid5ReconstructsMissingColumn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.Raid5ReconstructsMissingColumn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:311]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.Raid5TwoMissingColumnsUnrecoverable]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.Raid5TwoMissingColumnsUnrecoverable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.Raid5TwoMissingColumnsUnrecoverable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:339]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.MirroredReadFailover]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.MirroredReadFailover]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.MirroredReadFailover]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:396]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.MirroredBothCopiesFail]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.MirroredBothCopiesFail]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.MirroredBothCopiesFail]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:423]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.SpannedConcatenation]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.SpannedConcatenation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.SpannedConcatenation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:462]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.GptDynamicDiskCanonicalGuidsParse]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.GptDynamicDiskCanonicalGuidsParse]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.GptDynamicDiskCanonicalGuidsParse]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:597]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.GptDynamicDiskFabricatedGuidsRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.GptDynamicDiskFabricatedGuidsRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.GptDynamicDiskFabricatedGuidsRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:617]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.GptDynamicDiskTableClassifiesLdmEntries]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.GptDynamicDiskTableClassifiesLdmEntries]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.GptDynamicDiskTableClassifiesLdmEntries]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:628]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.GptDynamicDiskRealCanonicalGuidsParse]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.GptDynamicDiskRealCanonicalGuidsParse]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.GptDynamicDiskRealCanonicalGuidsParse]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:670]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.GptPathFindsMetadata4Kn]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.GptPathFindsMetadata4Kn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.GptPathFindsMetadata4Kn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:763]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.TocblockAtConfigPlus0]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.TocblockAtConfigPlus0]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.TocblockAtConfigPlus0]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:806]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.TocblockPrefersPlus2OnConflict]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.TocblockPrefersPlus2OnConflict]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.TocblockPrefersPlus2OnConflict]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:826]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.TocblockTailCopies]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.TocblockTailCopies]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.TocblockTailCopies]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:850]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.OversizeConfigRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.OversizeConfigRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.OversizeConfigRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:868]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.ConfigSizeExactlyOneMebibyteAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.ConfigSizeExactlyOneMebibyteAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.ConfigSizeExactlyOneMebibyteAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:890]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.ZeroInterleaveDefaultsTo64K]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Ldm.ZeroInterleaveDefaultsTo64K]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.ZeroInterleaveDefaultsTo64K]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:902]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.FindsScoresAndRejectsNoise]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.FindsScoresAndRejectsNoise]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.FindsScoresAndRejectsNoise]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:127]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.OverlapClassification]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.OverlapClassification]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.OverlapClassification]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:183]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.TwoHealthyDeletedPartitionsBothSurface]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.TwoHealthyDeletedPartitionsBothSurface]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.TwoHealthyDeletedPartitionsBothSurface]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:321]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.DamagedBpbFallbackMustNotSwallowPeer]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.DamagedBpbFallbackMustNotSwallowPeer]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.DamagedBpbFallbackMustNotSwallowPeer]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:337]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.Ext4SuperblockAnchorsDeletedPartition]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.Ext4SuperblockAnchorsDeletedPartition]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.Ext4SuperblockAnchorsDeletedPartition]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:439]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.Ext4BlockGroupCopyNotACandidateStart]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.Ext4BlockGroupCopyNotACandidateStart]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.Ext4BlockGroupCopyNotACandidateStart]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:470]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.HfsPlusVolumeHeaderAnchorsDeletedPartition]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.HfsPlusVolumeHeaderAnchorsDeletedPartition]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.HfsPlusVolumeHeaderAnchorsDeletedPartition]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:484]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.BtrfsSuperblockAnchorsDeletedPartition]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.BtrfsSuperblockAnchorsDeletedPartition]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.BtrfsSuperblockAnchorsDeletedPartition]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:500]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.ZfsLabelAnchorsDeletedPartition]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.ZfsLabelAnchorsDeletedPartition]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.ZfsLabelAnchorsDeletedPartition]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:515]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.TailBoundUsesCeiledSectorCount]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.TailBoundUsesCeiledSectorCount]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.TailBoundUsesCeiledSectorCount]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:531]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.RefsBootAnchorsDeletedPartition]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.RefsBootAnchorsDeletedPartition]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.RefsBootAnchorsDeletedPartition]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:595]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.SuperblockNoiseRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.SuperblockNoiseRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.SuperblockNoiseRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:614]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.LostBitlockerPartitionDiscovered]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.LostBitlockerPartitionDiscovered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.LostBitlockerPartitionDiscovered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:660]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.LostBitlockerOemNoise]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.LostBitlockerOemNoise]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.LostBitlockerOemNoise]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:700]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.LostBitlockerInsideExtendedRegionStillFound]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.LostBitlockerInsideExtendedRegionStillFound]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.LostBitlockerInsideExtendedRegionStillFound]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:738]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.FatBootWithOgGatesBecomesCandidate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.FatBootWithOgGatesBecomesCandidate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.FatBootWithOgGatesBecomesCandidate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:894]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.Fat32ExtentUsesDataAreaForContainment]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.Fat32ExtentUsesDataAreaForContainment]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.Fat32ExtentUsesDataAreaForContainment]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:912]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.LostCandidateReportsDataAreaOffset]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.LostCandidateReportsDataAreaOffset]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.LostCandidateReportsDataAreaOffset]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:948]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.FileRichCandidateOutranksCleanBoot]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.FileRichCandidateOutranksCleanBoot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.FileRichCandidateOutranksCleanBoot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:1066]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.TableMatchBonus500]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.TableMatchBonus500]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.TableMatchBonus500]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:1088]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.OverLargeDuplicateRescoredToFront]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.OverLargeDuplicateRescoredToFront]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.OverLargeDuplicateRescoredToFront]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:1117]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.ZeroSectorSizeFallsBackTo512]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostDiscovery.ZeroSectorSizeFallsBackTo512]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.ZeroSectorSizeFallsBackTo512]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:1170]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostPart.ForgedOemNoMftEvidenceRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostPart.ForgedOemNoMftEvidenceRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostPart.ForgedOemNoMftEvidenceRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:253]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostPart.TornPrimaryMftMirrorEvidenceScored]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostPart.TornPrimaryMftMirrorEvidenceScored]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostPart.TornPrimaryMftMirrorEvidenceScored]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:265]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostPart.HealthyCandidateMftEvidenceUnchanged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostPart.HealthyCandidateMftEvidenceUnchanged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostPart.HealthyCandidateMftEvidenceUnchanged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:288]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostFatGates.Spc256Rejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostFatGates.Spc256Rejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostFatGates.Spc256Rejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:803]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostFatGates.NumFatsZeroRejectedThreeAcceptedByDeepGates]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostFatGates.NumFatsZeroRejectedThreeAcceptedByDeepGates]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostFatGates.NumFatsZeroRejectedThreeAcceptedByDeepGates]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:816]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostFatGates.Fat32NameStringTornYieldsNoCandidate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostFatGates.Fat32NameStringTornYieldsNoCandidate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostFatGates.Fat32NameStringTornYieldsNoCandidate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:840]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostFatGates.Fat12SmallFatProbe]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostFatGates.Fat12SmallFatProbe]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostFatGates.Fat12SmallFatProbe]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:856]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostFatGates.Fat12ProbeSpfBoundIsExclusiveAt200]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostFatGates.Fat12ProbeSpfBoundIsExclusiveAt200]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostFatGates.Fat12ProbeSpfBoundIsExclusiveAt200]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:876]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostFatGates.Fat32GateStringIsCaseSensitive]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostFatGates.Fat32GateStringIsCaseSensitive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostFatGates.Fat32GateStringIsCaseSensitive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:884]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostScoring.MatchesLogRows]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostScoring.MatchesLogRows]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostScoring.MatchesLogRows]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:1043]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MountPoints2Decode.ParsesCleanPlacement]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=MountPoints2Decode.ParsesCleanPlacement]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MountPoints2Decode.ParsesCleanPlacement]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:286]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MountPoints2Decode.LowercaseHexAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=MountPoints2Decode.LowercaseHexAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MountPoints2Decode.LowercaseHexAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:294]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MountPoints2Decode.ShortBlobRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=MountPoints2Decode.ShortBlobRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MountPoints2Decode.ShortBlobRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:305]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MountPoints2Decode.MissingDelimiterRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=MountPoints2Decode.MissingDelimiterRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MountPoints2Decode.MissingDelimiterRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:320]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MountPoints2Decode.MisalignedStartRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=MountPoints2Decode.MisalignedStartRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MountPoints2Decode.MisalignedStartRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:330]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MountPoints2Decode.NonHexFieldRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=MountPoints2Decode.NonHexFieldRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MountPoints2Decode.NonHexFieldRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:337]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MountPoints2Decode.TruncatedHexStillParsesPrefix]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=MountPoints2Decode.TruncatedHexStillParsesPrefix]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MountPoints2Decode.TruncatedHexStillParsesPrefix]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:346]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MountPoints2Decode.SectorZeroIsTheFailureSentinel]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=MountPoints2Decode.SectorZeroIsTheFailureSentinel]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MountPoints2Decode.SectorZeroIsTheFailureSentinel]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:358]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MountPoints2Walk.ParsesPlacementsFromSyntheticHive]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=MountPoints2Walk.ParsesPlacementsFromSyntheticHive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MountPoints2Walk.ParsesPlacementsFromSyntheticHive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:368]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MountPoints2Walk.AbsentKeyYieldsZeroPlacements]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=MountPoints2Walk.AbsentKeyYieldsZeroPlacements]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MountPoints2Walk.AbsentKeyYieldsZeroPlacements]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:403]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MountPoints2Walk.ZeroBpsRefused]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=MountPoints2Walk.ZeroBpsRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MountPoints2Walk.ZeroBpsRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:416]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[MountPoints2Walk.CaseInsensitiveKeyPath]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=MountPoints2Walk.CaseInsensitiveKeyPath]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[MountPoints2Walk.CaseInsensitiveKeyPath]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:425]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RegisterChannel.FindsStrideMissedPartition]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RegisterChannel.FindsStrideMissedPartition]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RegisterChannel.FindsStrideMissedPartition]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:442]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RegisterChannel.ThreeSourceDedupSingleCandidate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RegisterChannel.ThreeSourceDedupSingleCandidate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RegisterChannel.ThreeSourceDedupSingleCandidate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:480]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RegisterChannel.SweepProposalWinsProvenance]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RegisterChannel.SweepProposalWinsProvenance]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RegisterChannel.SweepProposalWinsProvenance]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:502]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RegisterChannel.UnprobeablePlacementDropped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RegisterChannel.UnprobeablePlacementDropped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RegisterChannel.UnprobeablePlacementDropped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:517]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RegisterChannel.OutOfBoundsPlacementDropped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RegisterChannel.OutOfBoundsPlacementDropped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RegisterChannel.OutOfBoundsPlacementDropped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:530]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RegisterChannel.RegisterRecordsAreRevalidated]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RegisterChannel.RegisterRecordsAreRevalidated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RegisterChannel.RegisterRecordsAreRevalidated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:541]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RegisterChannel.EmptyHistoryIsThePreFixShape]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RegisterChannel.EmptyHistoryIsThePreFixShape]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RegisterChannel.EmptyHistoryIsThePreFixShape]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_mountpoints2.cpp:563]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Sha256.NistVectors]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Sha256.NistVectors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Sha256.NistVectors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_sha256.cpp:9]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Sha256.StreamedMatchesOneShot]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Sha256.StreamedMatchesOneShot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Sha256.StreamedMatchesOneShot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_sha256.cpp:28]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Sha256.Id128Truncation]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Sha256.Id128Truncation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Sha256.Id128Truncation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_sha256.cpp:43]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.Aes128Fips197KnownAnswer]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=AesXts.Aes128Fips197KnownAnswer]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.Aes128Fips197KnownAnswer]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:169]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.Aes256Fips197KnownAnswer]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=AesXts.Aes256Fips197KnownAnswer]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.Aes256Fips197KnownAnswer]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:182]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.Xts128CrossImplementationVector]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=AesXts.Xts128CrossImplementationVector]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.Xts128CrossImplementationVector]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:200]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.Xts256CrossImplementationVector]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=AesXts.Xts256CrossImplementationVector]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.Xts256CrossImplementationVector]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:216]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.Xts256WholeSectorVector]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=AesXts.Xts256WholeSectorVector]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.Xts256WholeSectorVector]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:234]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.WeakKeyHalvesRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=AesXts.WeakKeyHalvesRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.WeakKeyHalvesRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:269]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.RoundTripVariedLengths]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=AesXts.RoundTripVariedLengths]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.RoundTripVariedLengths]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:282]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.DistinctTweaksGiveDistinctCiphertexts]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=AesXts.DistinctTweaksGiveDistinctCiphertexts]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.DistinctTweaksGiveDistinctCiphertexts]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:306]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DecryptingDevice.ReadsPlaintextThroughWrapper]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DecryptingDevice.ReadsPlaintextThroughWrapper]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DecryptingDevice.ReadsPlaintextThroughWrapper]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:327]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DecryptingDevice.UnalignedAndCrossSectorReads]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DecryptingDevice.UnalignedAndCrossSectorReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DecryptingDevice.UnalignedAndCrossSectorReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:359]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DecryptingDevice.InnerErrorsPropagateVerbatim]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DecryptingDevice.InnerErrorsPropagateVerbatim]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DecryptingDevice.InnerErrorsPropagateVerbatim]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:404]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DecryptingDevice.BadKeyMaterialCreateFails]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DecryptingDevice.BadKeyMaterialCreateFails]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DecryptingDevice.BadKeyMaterialCreateFails]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:417]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BitLockerStack.EncryptedVolumeScansOnlyThroughDecryptDevice]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=BitLockerStack.EncryptedVolumeScansOnlyThroughDecryptDevice]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BitLockerStack.EncryptedVolumeScansOnlyThroughDecryptDevice]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:431]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.ShippedTableMatchesCorpusShape]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Dict.ShippedTableMatchesCorpusShape]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.ShippedTableMatchesCorpusShape]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:77]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.ShippedTableKeepsCorpusDuplicates]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Dict.ShippedTableKeepsCorpusDuplicates]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.ShippedTableKeepsCorpusDuplicates]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:101]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.ParseDictionaryGrammar]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Dict.ParseDictionaryGrammar]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.ParseDictionaryGrammar]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:131]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.ParseToleratesLfAndEmptyLines]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Dict.ParseToleratesLfAndEmptyLines]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.ParseToleratesLfAndEmptyLines]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:143]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.ParseRoundTripsTheCorpusGrammar]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Dict.ParseRoundTripsTheCorpusGrammar]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.ParseRoundTripsTheCorpusGrammar]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:154]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.ParseRejectsBadGrammar]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Dict.ParseRejectsBadGrammar]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.ParseRejectsBadGrammar]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:169]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.TryStopsAtFirstSuccessInShippedOrder]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Dict.TryStopsAtFirstSuccessInShippedOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.TryStopsAtFirstSuccessInShippedOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:190]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.UserCandidatesFollowShippedDictionary]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Dict.UserCandidatesFollowShippedDictionary]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.UserCandidatesFollowShippedDictionary]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:203]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.AllFailReportsEveryAttempt]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Dict.AllFailReportsEveryAttempt]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.AllFailReportsEveryAttempt]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:218]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.NullAttemptFunctionIsANoOp]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Dict.NullAttemptFunctionIsANoOp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.NullAttemptFunctionIsANoOp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:228]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.AttemptsLoggedThroughSeamWithoutPasswordMaterial]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Dict.AttemptsLoggedThroughSeamWithoutPasswordMaterial]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.AttemptsLoggedThroughSeamWithoutPasswordMaterial]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:234]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.ResumeDropsStaleCandidateAfterRelayout]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.ResumeDropsStaleCandidateAfterRelayout]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.ResumeDropsStaleCandidateAfterRelayout]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:223]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.ResumeKeepsValidCandidate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.ResumeKeepsValidCandidate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.ResumeKeepsValidCandidate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:270]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.DiskLayoutRecordRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.DiskLayoutRecordRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.DiskLayoutRecordRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:295]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.LayoutChangedFlagged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.LayoutChangedFlagged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.LayoutChangedFlagged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:334]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.SizeSourceFlagRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.SizeSourceFlagRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.SizeSourceFlagRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:390]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.CandidateGeometryRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.CandidateGeometryRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.CandidateGeometryRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:422]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.ArbiterStartOffDiskDropsAndUnknownKeeps]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.ArbiterStartOffDiskDropsAndUnknownKeeps]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.ArbiterStartOffDiskDropsAndUnknownKeeps]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:490]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.RoundTripKeepsDataAreaExtent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.RoundTripKeepsDataAreaExtent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.RoundTripKeepsDataAreaExtent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:525]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.V3SessionResumeReDerivesDataArea]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.V3SessionResumeReDerivesDataArea]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.V3SessionResumeReDerivesDataArea]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:547]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.RestoredWindowMatchesFreshDiscovery]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.RestoredWindowMatchesFreshDiscovery]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.RestoredWindowMatchesFreshDiscovery]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:574]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.EnrichedExtentIsIdempotentAcrossReSave]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.EnrichedExtentIsIdempotentAcrossReSave]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.EnrichedExtentIsIdempotentAcrossReSave]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:607]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.AsciiPassesThrough]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.AsciiPassesThrough]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.AsciiPassesThrough]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:28]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.LatinAccentDecodes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.LatinAccentDecodes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.LatinAccentDecodes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:34]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.CjkDecodesToCodePoints]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.CjkDecodesToCodePoints]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.CjkDecodesToCodePoints]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:39]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.AstralPlaneBecomesSurrogatePair]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.AstralPlaneBecomesSurrogatePair]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.AstralPlaneBecomesSurrogatePair]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:46]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.OverlongFormsRejectedAsReplacement]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.OverlongFormsRejectedAsReplacement]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.OverlongFormsRejectedAsReplacement]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:53]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.SurrogateHalvesRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.SurrogateHalvesRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.SurrogateHalvesRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:67]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.BeyondUnicodeRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.BeyondUnicodeRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.BeyondUnicodeRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:77]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.InvalidLeadBytesResyncOneByte]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.InvalidLeadBytesResyncOneByte]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.InvalidLeadBytesResyncOneByte]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:86]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.TruncatedTailBecomesReplacement]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.TruncatedTailBecomesReplacement]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.TruncatedTailBecomesReplacement]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:93]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.MaximalSubpartReplacement]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.MaximalSubpartReplacement]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.MaximalSubpartReplacement]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:108]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.EmbeddedNulDecodes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.EmbeddedNulDecodes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.EmbeddedNulDecodes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:130]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.RoundTripThroughNodeUtf8]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.RoundTripThroughNodeUtf8]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.RoundTripThroughNodeUtf8]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:136]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.InvalidByteLosslessMapping]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.InvalidByteLosslessMapping]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.InvalidByteLosslessMapping]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:174]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.LosslessRoundTripRecoversRawBytes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.LosslessRoundTripRecoversRawBytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.LosslessRoundTripRecoversRawBytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:193]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.ZfsDecoderStaysReplacement]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.ZfsDecoderStaysReplacement]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.ZfsDecoderStaysReplacement]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:217]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.WideToUtf8EncodesNonAscii]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.WideToUtf8EncodesNonAscii]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.WideToUtf8EncodesNonAscii]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:228]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.WideToUtf8SurrogatePairAndLoneHalf]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.WideToUtf8SurrogatePairAndLoneHalf]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.WideToUtf8SurrogatePairAndLoneHalf]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:246]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.CliFilterUtf8MatchesDecodedNames]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Text.CliFilterUtf8MatchesDecodedNames]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.CliFilterUtf8MatchesDecodedNames]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:263]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.GrammarValidationAndSpecificity]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.GrammarValidationAndSpecificity]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.GrammarValidationAndSpecificity]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:257]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.CarvesMidSectorStart]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.CarvesMidSectorStart]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.CarvesMidSectorStart]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:442]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.SecondPassFullyDeduped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.SecondPassFullyDeduped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.SecondPassFullyDeduped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:478]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.VideoPostProcessorHookInvoked]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.VideoPostProcessorHookInvoked]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.VideoPostProcessorHookInvoked]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:549]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.HookDegradesWhenModuleAbsent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.HookDegradesWhenModuleAbsent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.HookDegradesWhenModuleAbsent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:597]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.RegistryF8Gate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.RegistryF8Gate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.RegistryF8Gate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:614]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.PstAndRar5Grammar]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.PstAndRar5Grammar]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.PstAndRar5Grammar]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:620]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ExtendedFormatsGrammar]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.ExtendedFormatsGrammar]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ExtendedFormatsGrammar]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:715]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.PdfEofLastWinsAndFrameFloors]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.PdfEofLastWinsAndFrameFloors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.PdfEofLastWinsAndFrameFloors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:925]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.RawSignLayerHeaderOnlyAndSharedMagic]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.RawSignLayerHeaderOnlyAndSharedMagic]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.RawSignLayerHeaderOnlyAndSharedMagic]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1017]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.RawSignTailPairWithinFormat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.RawSignTailPairWithinFormat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.RawSignTailPairWithinFormat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1071]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.UnreadableWindowNotCountedAsScanned]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.UnreadableWindowNotCountedAsScanned]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.UnreadableWindowNotCountedAsScanned]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1217]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ZipCdAnchorCarvesDestroyedHead]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.ZipCdAnchorCarvesDestroyedHead]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ZipCdAnchorCarvesDestroyedHead]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1246]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ZipIndexlessLfhChainSalvage]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.ZipIndexlessLfhChainSalvage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ZipIndexlessLfhChainSalvage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1269]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ZipSalvageAbsorbsDataSectors]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.ZipSalvageAbsorbsDataSectors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ZipSalvageAbsorbsDataSectors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1304]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.Zip64ExtraFieldContinuesChain]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.Zip64ExtraFieldContinuesChain]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.Zip64ExtraFieldContinuesChain]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1341]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ZipCleanArchiveNotInflatedByZeroSlack]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.ZipCleanArchiveNotInflatedByZeroSlack]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ZipCleanArchiveNotInflatedByZeroSlack]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1396]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.BeTiffAndOrfVariantsCarve]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.BeTiffAndOrfVariantsCarve]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.BeTiffAndOrfVariantsCarve]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1421]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.OverlapScorePrefersHigherConfidence]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.OverlapScorePrefersHigherConfidence]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.OverlapScorePrefersHigherConfidence]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1492]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.TiffVsRawTiebreak]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.TiffVsRawTiebreak]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.TiffVsRawTiebreak]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1548]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.PstUnknownVersionAcceptedAtFloor]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.PstUnknownVersionAcceptedAtFloor]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.PstUnknownVersionAcceptedAtFloor]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1600]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.JpegSof0PrecisionGate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.JpegSof0PrecisionGate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.JpegSof0PrecisionGate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1630]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.SlicedDeviceSecondPassFullyDedupedAndAbsolute]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.SlicedDeviceSecondPassFullyDedupedAndAbsolute]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.SlicedDeviceSecondPassFullyDedupedAndAbsolute]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1669]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.CarvedNodesAreDeleted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.CarvedNodesAreDeleted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.CarvedNodesAreDeleted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5013]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.RawSignCarvedNodesAreDeleted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.RawSignCarvedNodesAreDeleted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.RawSignCarvedNodesAreDeleted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5050]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.DeletedOnlyQueryIncludesCarvedFiles]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.DeletedOnlyQueryIncludesCarvedFiles]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.DeletedOnlyQueryIncludesCarvedFiles]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5075]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.CarvedNodesCarryScanTimestamp]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.CarvedNodesCarryScanTimestamp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.CarvedNodesCarryScanTimestamp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5109]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.RawSignCarvedNodesCarryScanTimestamp]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.RawSignCarvedNodesCarryScanTimestamp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.RawSignCarvedNodesCarryScanTimestamp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5146]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ScanTimestampSurvivesSessionRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.ScanTimestampSurvivesSessionRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ScanTimestampSurvivesSessionRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5177]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ZipInnerEntryInventoryEmitted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.ZipInnerEntryInventoryEmitted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ZipInnerEntryInventoryEmitted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5381]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ZipIndexlessEntryInventory]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.ZipIndexlessEntryInventory]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ZipIndexlessEntryInventory]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5418]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.Rar4InnerEntryInventory]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.Rar4InnerEntryInventory]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.Rar4InnerEntryInventory]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5465]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ProvenanceChecksumAndAnchorsPersist]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.ProvenanceChecksumAndAnchorsPersist]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ProvenanceChecksumAndAnchorsPersist]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5983]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.TailAnchorCarriedOnTailPairedHits]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.TailAnchorCarriedOnTailPairedHits]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.TailAnchorCarriedOnTailPairedHits]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6038]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.SqliteRealHeaderByteOrder]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.SqliteRealHeaderByteOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.SqliteRealHeaderByteOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6078]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.SqliteInvalidEncodingRejects]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.SqliteInvalidEncodingRejects]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.SqliteInvalidEncodingRejects]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6107]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.SqliteLittleEndianShapeRejects]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.SqliteLittleEndianShapeRejects]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.SqliteLittleEndianShapeRejects]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6129]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.SqliteFcpeventSubtypeRelabeled]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.SqliteFcpeventSubtypeRelabeled]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.SqliteFcpeventSubtypeRelabeled]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6163]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.SqliteGenericLabelUnchangedWithoutMarker]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.SqliteGenericLabelUnchangedWithoutMarker]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.SqliteGenericLabelUnchangedWithoutMarker]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6193]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.BitmapSkipsUsedClusters]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.BitmapSkipsUsedClusters]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.BitmapSkipsUsedClusters]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6223]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.NoBitmapCarvesEverything]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.NoBitmapCarvesEverything]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.NoBitmapCarvesEverything]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6267]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.FormatDisabledSkipsSignature]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.FormatDisabledSkipsSignature]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.FormatDisabledSkipsSignature]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6314]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.FormatDisabledRawSignsToo]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.FormatDisabledRawSignsToo]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.FormatDisabledRawSignsToo]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6350]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.FilterExemptSetAlwaysOn]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.FilterExemptSetAlwaysOn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.FilterExemptSetAlwaysOn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6457]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.FilterExemptSetCoversTypedRows]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.FilterExemptSetCoversTypedRows]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.FilterExemptSetCoversTypedRows]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6531]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.AllFormatsEnabledByDefault]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.AllFormatsEnabledByDefault]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.AllFormatsEnabledByDefault]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6575]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.WindowServingEliminatesPerCandidateRereads]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.WindowServingEliminatesPerCandidateRereads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.WindowServingEliminatesPerCandidateRereads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6689]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.WindowServingServesStraddlingReadsCorrectly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.WindowServingServesStraddlingReadsCorrectly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.WindowServingServesStraddlingReadsCorrectly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6726]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.Mp4NamedByDurationPrefix]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.Mp4NamedByDurationPrefix]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.Mp4NamedByDurationPrefix]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6771]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.MovNamedByDurationPrefix]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.MovNamedByDurationPrefix]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.MovNamedByDurationPrefix]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6799]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.MovV1MvhdDurationRead]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.MovV1MvhdDurationRead]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.MovV1MvhdDurationRead]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6818]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ReconstructedMultiVolumeGrouping]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.ReconstructedMultiVolumeGrouping]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ReconstructedMultiVolumeGrouping]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6841]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ReconstructedUnattributedPositionStaysAtRoot]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.ReconstructedUnattributedPositionStaysAtRoot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ReconstructedUnattributedPositionStaysAtRoot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6905]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ReconstructedSingleDefaultGroupName]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.ReconstructedSingleDefaultGroupName]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ReconstructedSingleDefaultGroupName]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6941]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.OgNamingUniversalExtCounter]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.OgNamingUniversalExtCounter]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.OgNamingUniversalExtCounter]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6966]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.OgNamingJpgSofDimensions]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.OgNamingJpgSofDimensions]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.OgNamingJpgSofDimensions]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7000]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.JpegExifPrefixCappedAtThirtyNine]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.JpegExifPrefixCappedAtThirtyNine]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.JpegExifPrefixCappedAtThirtyNine]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7023]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.OgNamingJpgExifModelAndSoftware]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.OgNamingJpgExifModelAndSoftware]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.OgNamingJpgExifModelAndSoftware]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7125]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.OgNamingMp3Id3Title]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.OgNamingMp3Id3Title]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.OgNamingMp3Id3Title]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7244]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.OgNamingPdfCreatorAndDatePrefixes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.OgNamingPdfCreatorAndDatePrefixes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.OgNamingPdfCreatorAndDatePrefixes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7299]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.OgNamingDocxCreatedTimestamp]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.OgNamingDocxCreatedTimestamp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.OgNamingDocxCreatedTimestamp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7415]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.OgNamingDocxCreatorComposesWithCreated]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.OgNamingDocxCreatorComposesWithCreated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.OgNamingDocxCreatorComposesWithCreated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7511]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.OgNamingCounterKeyTwoClasses]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.OgNamingCounterKeyTwoClasses]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.OgNamingCounterKeyTwoClasses]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7608]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.OgNamingSessionWideCounters]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.OgNamingSessionWideCounters]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.OgNamingSessionWideCounters]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7699]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.OgNamingWavInamTitle]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.OgNamingWavInamTitle]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.OgNamingWavInamTitle]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7726]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.OgNamingCounterPast99999Unpadded]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.OgNamingCounterPast99999Unpadded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.OgNamingCounterPast99999Unpadded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7777]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.OgNamingZipFirstCentralDirectoryEntry]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.OgNamingZipFirstCentralDirectoryEntry]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.OgNamingZipFirstCentralDirectoryEntry]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7792]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.FileNnnnFallbackRemoved]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.FileNnnnFallbackRemoved]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.FileNnnnFallbackRemoved]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7817]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ZeroSectorSkipStats]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.ZeroSectorSkipStats]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ZeroSectorSkipStats]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7838]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.UserSignatureProbedAtStride]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.UserSignatureProbedAtStride]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.UserSignatureProbedAtStride]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7957]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.UserSignatureYieldsToBuiltInAtSamePosition]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.UserSignatureYieldsToBuiltInAtSamePosition]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.UserSignatureYieldsToBuiltInAtSamePosition]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8018]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.UserSignYieldsToRawSignTableAtSamePosition]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.UserSignYieldsToRawSignTableAtSamePosition]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.UserSignYieldsToRawSignTableAtSamePosition]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8059]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.DisabledFormatGatesUserSignLayer]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.DisabledFormatGatesUserSignLayer]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.DisabledFormatGatesUserSignLayer]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8141]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.UserSignFilesCountInCarvedFound]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.UserSignFilesCountInCarvedFound]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.UserSignFilesCountInCarvedFound]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8196]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.WindowBufferReusedByValidatorChecksumAndHash]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.WindowBufferReusedByValidatorChecksumAndHash]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.WindowBufferReusedByValidatorChecksumAndHash]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9061]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.BoundaryStraddlingCandidateSplitsReadsCorrectly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.BoundaryStraddlingCandidateSplitsReadsCorrectly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.BoundaryStraddlingCandidateSplitsReadsCorrectly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9091]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.SkipsFsAllocatedRegions]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.SkipsFsAllocatedRegions]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.SkipsFsAllocatedRegions]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9142]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.StructuredBoundaryPartialOverlap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.StructuredBoundaryPartialOverlap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.StructuredBoundaryPartialOverlap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9176]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.OccupancyGateJumpsWholeInterval]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveEngine.OccupancyGateJumpsWholeInterval]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.OccupancyGateJumpsWholeInterval]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9210]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.StatsPerFormatTally]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.StatsPerFormatTally]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.StatsPerFormatTally]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:357]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Mp4RealFtypIsom]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Mp4RealFtypIsom]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Mp4RealFtypIsom]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1767]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MovRealFtypQt]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.MovRealFtypQt]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MovRealFtypQt]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1788]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.HeicRealFtypHeic]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.HeicRealFtypHeic]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.HeicRealFtypHeic]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1804]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.M4aRealFtyp]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.M4aRealFtyp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.M4aRealFtyp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1820]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MovMoovWithoutFtyp]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.MovMoovWithoutFtyp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MovMoovWithoutFtyp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1836]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.RegistrySelfProbeReachesEveryFormat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.RegistrySelfProbeReachesEveryFormat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.RegistrySelfProbeReachesEveryFormat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1861]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.X3fExtentIsSecdPlusFixedTail]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.X3fExtentIsSecdPlusFixedTail]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.X3fExtentIsSecdPlusFixedTail]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2325]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.X3fNoSectionDirectorySalvagesWalkedPrefix]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.X3fNoSectionDirectorySalvagesWalkedPrefix]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.X3fNoSectionDirectorySalvagesWalkedPrefix]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2350]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.X3fGarbageCountIsIrrelevantToExtent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.X3fGarbageCountIsIrrelevantToExtent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.X3fGarbageCountIsIrrelevantToExtent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2379]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.S007CommonFormatsCarve]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.S007CommonFormatsCarve]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.S007CommonFormatsCarve]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2872]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.S007ValidatorsRejectNoise]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.S007ValidatorsRejectNoise]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.S007ValidatorsRejectNoise]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2929]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.S007TruncatedSalvageSemantics]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.S007TruncatedSalvageSemantics]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.S007TruncatedSalvageSemantics]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2991]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.S007NicheBatch1Carve]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.S007NicheBatch1Carve]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.S007NicheBatch1Carve]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3032]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.S007NicheBatch1RejectsNoise]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.S007NicheBatch1RejectsNoise]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.S007NicheBatch1RejectsNoise]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3141]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.S007M081CiwMrmLowdocHlpOgGrammar]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.S007M081CiwMrmLowdocHlpOgGrammar]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.S007M081CiwMrmLowdocHlpOgGrammar]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3281]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.S007NicheBatch2School1Carve]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.S007NicheBatch2School1Carve]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.S007NicheBatch2School1Carve]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3379]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.M090DmgKolyFooterExtent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.M090DmgKolyFooterExtent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.M090DmgKolyFooterExtent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3442]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.M090DmgKolyUnalignedSalvage]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.M090DmgKolyUnalignedSalvage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.M090DmgKolyUnalignedSalvage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3485]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.M090DmgKolyZeroTailGateAndCap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.M090DmgKolyZeroTailGateAndCap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.M090DmgKolyZeroTailGateAndCap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3517]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.M084AccdbSecondSignatureCarves]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.M084AccdbSecondSignatureCarves]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.M084AccdbSecondSignatureCarves]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3556]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.M091MdbPageChainWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.M091MdbPageChainWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.M091MdbPageChainWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3600]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.S007NicheBatch2School1RejectsNoise]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.S007NicheBatch2School1RejectsNoise]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.S007NicheBatch2School1RejectsNoise]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3659]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.WmfPlaceableOgGrammarCarves]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.WmfPlaceableOgGrammarCarves]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.WmfPlaceableOgGrammarCarves]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3715]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.WmfPlaceableOldCoreGrammarRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.WmfPlaceableOldCoreGrammarRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.WmfPlaceableOldCoreGrammarRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3749]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.WmfPlaceableTwentyByteVariantRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.WmfPlaceableTwentyByteVariantRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.WmfPlaceableTwentyByteVariantRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3793]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.WmfStdOgGatesAcceptNonCanonicalHeaders]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.WmfStdOgGatesAcceptNonCanonicalHeaders]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.WmfStdOgGatesAcceptNonCanonicalHeaders]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3838]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MpegTs188PacketStreamAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.MpegTs188PacketStreamAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MpegTs188PacketStreamAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3876]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.M2Ts192PacketStreamAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.M2Ts192PacketStreamAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.M2Ts192PacketStreamAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3909]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TsBelowMinimumExtentRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TsBelowMinimumExtentRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TsBelowMinimumExtentRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3945]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TsSyncLossCommitsRun]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TsSyncLossCommitsRun]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TsSyncLossCommitsRun]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3975]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TsNullPacketTolerated]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TsNullPacketTolerated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TsNullPacketTolerated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4008]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TsSparseSyncNoiseRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TsSparseSyncNoiseRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TsSparseSyncNoiseRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4044]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.HtmlAsciiLowerUpperDoctypeAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.HtmlAsciiLowerUpperDoctypeAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.HtmlAsciiLowerUpperDoctypeAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4078]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.HtmlUtf16BomChainAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.HtmlUtf16BomChainAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.HtmlUtf16BomChainAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4130]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.HtmlNoEndTagFallsBackBounded]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.HtmlNoEndTagFallsBackBounded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.HtmlNoEndTagFallsBackBounded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4167]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.HtmlNonHtmlPrefixesRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.HtmlNonHtmlPrefixesRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.HtmlNonHtmlPrefixesRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4204]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.SvgDeclaredSvgAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.SvgDeclaredSvgAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.SvgDeclaredSvgAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4246]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.SvgBomFormAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.SvgBomFormAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.SvgBomFormAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4285]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.SvgDirectKeysAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.SvgDirectKeysAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.SvgDirectKeysAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4321]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.GenericXmlFallbackId]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.GenericXmlFallbackId]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.GenericXmlFallbackId]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4367]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.XmlNoMarkerNoSvgRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.XmlNoMarkerNoSvgRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.XmlNoMarkerNoSvgRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4403]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MobiBookMobiPairAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.MobiBookMobiPairAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MobiBookMobiPairAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4435]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MobiOtherPalmCreatorsRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.MobiOtherPalmCreatorsRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MobiOtherPalmCreatorsRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4482]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MobiNoEndTagFallsBackBounded]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.MobiNoEndTagFallsBackBounded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MobiNoEndTagFallsBackBounded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4509]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.S007NicheBatch1SalvageSemantics]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.S007NicheBatch1SalvageSemantics]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.S007NicheBatch1SalvageSemantics]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4551]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.S007NicheBatch1SharedKeyDiscriminates]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.S007NicheBatch1SharedKeyDiscriminates]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.S007NicheBatch1SharedKeyDiscriminates]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4609]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.AsfTornCountFieldStillEmitted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.AsfTornCountFieldStillEmitted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.AsfTornCountFieldStillEmitted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4661]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.AsfZeroHeaderSizeEmitsThirtyByteSalvage]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.AsfZeroHeaderSizeEmitsThirtyByteSalvage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.AsfZeroHeaderSizeEmitsThirtyByteSalvage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4690]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.AsfFilePropsDeclaredSizeDrivesExtent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.AsfFilePropsDeclaredSizeDrivesExtent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.AsfFilePropsDeclaredSizeDrivesExtent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4711]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.AsfIntactFileExtentUnchanged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.AsfIntactFileExtentUnchanged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.AsfIntactFileExtentUnchanged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4739]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.AsfCarveExtentCoversDataObjects]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.AsfCarveExtentCoversDataObjects]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.AsfCarveExtentCoversDataObjects]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4763]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.AsfCarveStopsAtGrammarBreak]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.AsfCarveStopsAtGrammarBreak]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.AsfCarveStopsAtGrammarBreak]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4787]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.AsfCarveTornTailSalvagesLastCompleteObject]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.AsfCarveTornTailSalvagesLastCompleteObject]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.AsfCarveTornTailSalvagesLastCompleteObject]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4811]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.AsfHeaderCountGatesUnchanged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.AsfHeaderCountGatesUnchanged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.AsfHeaderCountGatesUnchanged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4835]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Fh11DetectedAndWalked]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Fh11DetectedAndWalked]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Fh11DetectedAndWalked]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4861]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Fh11RejectsBareKeyBytes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Fh11RejectsBareKeyBytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Fh11RejectsBareKeyBytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4893]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Fh11BoundedExtentPinned]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Fh11BoundedExtentPinned]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Fh11BoundedExtentPinned]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4918]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.CptGenerationsDetected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.CptGenerationsDetected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.CptGenerationsDetected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4949]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.CptRejectsTruncatedHeader]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.CptRejectsTruncatedHeader]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.CptRejectsTruncatedHeader]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:4983]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.ZipTruncatedHeadRecovered]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.ZipTruncatedHeadRecovered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.ZipTruncatedHeadRecovered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5277]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.ZipLfhSanityGates]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.ZipLfhSanityGates]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.ZipLfhSanityGates]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5306]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MkvDocTypeWebmAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.MkvDocTypeWebmAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MkvDocTypeWebmAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5595]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MkvDocTypeExoticRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.MkvDocTypeExoticRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MkvDocTypeExoticRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5613]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MkvElementBoundsGate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.MkvElementBoundsGate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MkvElementBoundsGate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5629]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.HeifSiblingBrandLabeledHeif]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.HeifSiblingBrandLabeledHeif]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.HeifSiblingBrandLabeledHeif]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5650]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.IiuTiffEpValidatedByTiffGrammar]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.IiuTiffEpValidatedByTiffGrammar]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.IiuTiffEpValidatedByTiffGrammar]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5675]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.IiuRw2FallbackWhenTiffEpRejects]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.IiuRw2FallbackWhenTiffEpRejects]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.IiuRw2FallbackWhenTiffEpRejects]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5697]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.RafVariantHeaderAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.RafVariantHeaderAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.RafVariantHeaderAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5716]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.RafRealAsciiVersionAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.RafRealAsciiVersionAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.RafRealAsciiVersionAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5742]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.JpegFourthByteOutsideOgSet]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.JpegFourthByteOutsideOgSet]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.JpegFourthByteOutsideOgSet]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5770]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.JpegTruncatedTailSalvagedAtSofEstimate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.JpegTruncatedTailSalvagedAtSofEstimate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.JpegTruncatedTailSalvagedAtSofEstimate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5808]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.JpegTruncatedTailWithoutSofSalvagesAtCapTenth]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.JpegTruncatedTailWithoutSofSalvagesAtCapTenth]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.JpegTruncatedTailWithoutSofSalvagesAtCapTenth]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5843]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.JpegCompleteFileStopsAtEoiNotSofEstimate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.JpegCompleteFileStopsAtEoiNotSofEstimate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.JpegCompleteFileStopsAtEoiNotSofEstimate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5880]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.CabLowercaseMagic]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.CabLowercaseMagic]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.CabLowercaseMagic]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:5944]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.SpecialRejectPlainAcceptRefusedByName]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.SpecialRejectPlainAcceptRefusedByName]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.SpecialRejectPlainAcceptRefusedByName]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:6384]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.UserSignYieldsToRawSignTableAtSamePosition]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.UserSignYieldsToRawSignTableAtSamePosition]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.UserSignYieldsToRawSignTableAtSamePosition]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8295]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.UserSignOverlapFormatsAllYieldToBuiltin]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.UserSignOverlapFormatsAllYieldToBuiltin]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.UserSignOverlapFormatsAllYieldToBuiltin]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8349]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.UserSignBuiltinFloorGateSuppressesUserSignToo]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.UserSignBuiltinFloorGateSuppressesUserSignToo]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.UserSignBuiltinFloorGateSuppressesUserSignToo]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8420]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.GrammarRejectedMagicNotReclaimedByUserSign]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.GrammarRejectedMagicNotReclaimedByUserSign]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.GrammarRejectedMagicNotReclaimedByUserSign]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8465]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.UserSignStillFiresWhenNoBuiltinMatches]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.UserSignStillFiresWhenNoBuiltinMatches]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.UserSignStillFiresWhenNoBuiltinMatches]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8518]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.DisabledFormatsGatesUserSignLayer]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.DisabledFormatsGatesUserSignLayer]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.DisabledFormatsGatesUserSignLayer]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8553]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.DisabledFormatsGatesUserOnlyExtension]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.DisabledFormatsGatesUserOnlyExtension]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.DisabledFormatsGatesUserOnlyExtension]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8615]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.DisabledOtherFormatsLeaveUserSignsOn]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.DisabledOtherFormatsLeaveUserSignsOn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.DisabledOtherFormatsLeaveUserSignsOn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8651]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.CplWplTwoLayerCollisionsStayBuiltin]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.CplWplTwoLayerCollisionsStayBuiltin]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.CplWplTwoLayerCollisionsStayBuiltin]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8689]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Batch1OgCapsAreTheRegistryRows]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Batch1OgCapsAreTheRegistryRows]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Batch1OgCapsAreTheRegistryRows]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9240]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TsM2TsCapIsTheOg200MB]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TsM2TsCapIsTheOg200MB]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TsM2TsCapIsTheOg200MB]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9343]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TsStreamClampedAt200MB]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TsStreamClampedAt200MB]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TsStreamClampedAt200MB]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9375]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MidBoundTenMegabytes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.MidBoundTenMegabytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MidBoundTenMegabytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9466]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.SitMinimumIsTheOgFloor]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.SitMinimumIsTheOgFloor]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.SitMinimumIsTheOgFloor]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9496]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Jp2BrandMap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Jp2BrandMap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Jp2BrandMap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9585]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Jp2SignaturePayloadGate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Jp2SignaturePayloadGate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Jp2SignaturePayloadGate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9621]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Jp2UncheckedBoxLength]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Jp2UncheckedBoxLength]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Jp2UncheckedBoxLength]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9639]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Jp2UnknownBrandRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Jp2UnknownBrandRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Jp2UnknownBrandRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9666]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Jp2BoxWalkExtent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Jp2BoxWalkExtent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Jp2BoxWalkExtent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9683]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Jp2EocScan]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Jp2EocScan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Jp2EocScan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9705]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Jp2Jp2cFinalAtSliceEdgeFallsBackBounded]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Jp2Jp2cFinalAtSliceEdgeFallsBackBounded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Jp2Jp2cFinalAtSliceEdgeFallsBackBounded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9733]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.LzhMethodFamilyAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.LzhMethodFamilyAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.LzhMethodFamilyAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9782]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.LzhEntryChainExtent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.LzhEntryChainExtent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.LzhEntryChainExtent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9810]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.LzhBrokenChainSalvagesLeadingEntries]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.LzhBrokenChainSalvagesLeadingEntries]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.LzhBrokenChainSalvagesLeadingEntries]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9834]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.LzhLh5HeaderOnlyStridePinned]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.LzhLh5HeaderOnlyStridePinned]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.LzhLh5HeaderOnlyStridePinned]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9859]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.LzhExtFlagSuppressesTheCrcStride]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.LzhExtFlagSuppressesTheCrcStride]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.LzhExtFlagSuppressesTheCrcStride]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9885]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.LzhWrongDashByteRejects]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.LzhWrongDashByteRejects]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.LzhWrongDashByteRejects]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9910]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TextSchoolVcfAcceptedAndExtent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TextSchoolVcfAcceptedAndExtent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TextSchoolVcfAcceptedAndExtent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9945]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TextSchoolVcfMultiCardContinuation]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TextSchoolVcfMultiCardContinuation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TextSchoolVcfMultiCardContinuation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9968]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TextSchoolVcfNoEndTagFallsBackBounded]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TextSchoolVcfNoEndTagFallsBackBounded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TextSchoolVcfNoEndTagFallsBackBounded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9991]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TextSchoolGpxAcceptedAndExtent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TextSchoolGpxAcceptedAndExtent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TextSchoolGpxAcceptedAndExtent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10010]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TextSchoolGpxRootBeforeWindowRejects]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TextSchoolGpxRootBeforeWindowRejects]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TextSchoolGpxRootBeforeWindowRejects]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10038]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TextSchoolOspAcceptedAndRejectOnMissing]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TextSchoolOspAcceptedAndRejectOnMissing]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TextSchoolOspAcceptedAndRejectOnMissing]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10056]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TextSchoolPosAcceptedAndExtent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TextSchoolPosAcceptedAndExtent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TextSchoolPosAcceptedAndExtent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10090]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TextSchoolChfpAcceptedAndExtent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TextSchoolChfpAcceptedAndExtent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TextSchoolChfpAcceptedAndExtent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10115]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TextSchoolChfpRootOffAnchorRejects]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TextSchoolChfpRootOffAnchorRejects]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TextSchoolChfpRootOffAnchorRejects]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10146]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.TextSchoolVcfInnerCardIsSameFile]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.TextSchoolVcfInnerCardIsSameFile]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.TextSchoolVcfInnerCardIsSameFile]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10166]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.SvgDeclaredDocumentCarvesOnce]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.SvgDeclaredDocumentCarvesOnce]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.SvgDeclaredDocumentCarvesOnce]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10192]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.GpxBomFormCarvesOnce]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.GpxBomFormCarvesOnce]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.GpxBomFormCarvesOnce]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10217]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.DxfCanonicalRequiresFullSixteenBytePattern]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.DxfCanonicalRequiresFullSixteenBytePattern]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.DxfCanonicalRequiresFullSixteenBytePattern]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10248]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Dxf999CommentHeaderLfCarves]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Dxf999CommentHeaderLfCarves]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Dxf999CommentHeaderLfCarves]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10292]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Dxf999CommentHeaderCrLfCarves]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Dxf999CommentHeaderCrLfCarves]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Dxf999CommentHeaderCrLfCarves]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10319]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Dxf999RejectsNonCommentContinuation]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Dxf999RejectsNonCommentContinuation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Dxf999RejectsNonCommentContinuation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10341]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Dxf999InteriorCommentOfCanonicalFileDoesNotDoubleCarve]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.Dxf999InteriorCommentOfCanonicalFileDoesNotDoubleCarve]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Dxf999InteriorCommentOfCanonicalFileDoesNotDoubleCarve]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10368]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.OoxmlRelsPathSplit]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.OoxmlRelsPathSplit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.OoxmlRelsPathSplit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10456]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.ZipEntriesReorderedStillTyped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.ZipEntriesReorderedStillTyped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.ZipEntriesReorderedStillTyped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10510]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.OdfEpubTypedByMimetype]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.OdfEpubTypedByMimetype]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.OdfEpubTypedByMimetype]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10531]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.IworkAndCdrTypedByEntryName]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.IworkAndCdrTypedByEntryName]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.IworkAndCdrTypedByEntryName]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10561]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.OfficeUnrefinableStaysDocxAndPlainZipStaysZip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.OfficeUnrefinableStaysDocxAndPlainZipStaysZip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.OfficeUnrefinableStaysDocxAndPlainZipStaysZip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10588]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.ZipTypingXmindContentBranch]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.ZipTypingXmindContentBranch]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.ZipTypingXmindContentBranch]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10612]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.ZipTypingXmindNeedleMissStaysGeneric]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.ZipTypingXmindNeedleMissStaysGeneric]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.ZipTypingXmindNeedleMissStaysGeneric]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10654]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.ZipTypingDocumentToolEntryNames]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.ZipTypingDocumentToolEntryNames]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.ZipTypingDocumentToolEntryNames]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10673]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.ZipTypingLaterEntryOverwritesEarlierStamp]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.ZipTypingLaterEntryOverwritesEarlierStamp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.ZipTypingLaterEntryOverwritesEarlierStamp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10718]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.ZipTypingUnrelatedEntriesStayGeneric]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.ZipTypingUnrelatedEntriesStayGeneric]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.ZipTypingUnrelatedEntriesStayGeneric]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10740]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.ZipTypedIdDisableGateSuppressesTyping]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.ZipTypedIdDisableGateSuppressesTyping]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.ZipTypedIdDisableGateSuppressesTyping]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10760]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MultiEntryZipCarvesOnce]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.MultiEntryZipCarvesOnce]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MultiEntryZipCarvesOnce]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10784]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.DbxPlainMagicCarvesWithDeclaredSize]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.DbxPlainMagicCarvesWithDeclaredSize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.DbxPlainMagicCarvesWithDeclaredSize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10829]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.DbxAllFourGuidsCarve]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.DbxAllFourGuidsCarve]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.DbxAllFourGuidsCarve]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10857]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.DbxJmf6VariantCarves]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.DbxJmf6VariantCarves]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.DbxJmf6VariantCarves]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10883]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.DbxJmf6WrongVersionWordRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.DbxJmf6WrongVersionWordRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.DbxJmf6WrongVersionWordRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10910]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.DbxMarkedFormZeroSectorWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.DbxMarkedFormZeroSectorWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.DbxMarkedFormZeroSectorWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10933]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.DbxMarkedFormOddPrefixRoundsEven]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.DbxMarkedFormOddPrefixRoundsEven]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.DbxMarkedFormOddPrefixRoundsEven]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10969]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.DbxRejectsWrongGuid]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.DbxRejectsWrongGuid]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.DbxRejectsWrongGuid]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:10999]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.DbxRejectsTruncatedMarkedPattern]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.DbxRejectsTruncatedMarkedPattern]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.DbxRejectsTruncatedMarkedPattern]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11024]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.DbxRegistryCarriesAllThreeKeys]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.DbxRegistryCarriesAllThreeKeys]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.DbxRegistryCarriesAllThreeKeys]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11046]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.CslSizeFieldExtendsPastCap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.CslSizeFieldExtendsPastCap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.CslSizeFieldExtendsPastCap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11073]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.CslCd01OutOfCapDeclinedIsDropped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.CslCd01OutOfCapDeclinedIsDropped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.CslCd01OutOfCapDeclinedIsDropped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11112]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.CslShortCd01DeclinedIsDropped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.CslShortCd01DeclinedIsDropped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.CslShortCd01DeclinedIsDropped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11147]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.SvgLegUnaffectedByCslBranch]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.SvgLegUnaffectedByCslBranch]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.SvgLegUnaffectedByCslBranch]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11178]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.HikPackSystemHeaderPrivateStreamAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.HikPackSystemHeaderPrivateStreamAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.HikPackSystemHeaderPrivateStreamAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11241]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.HikDirectPrivateStreamAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.HikDirectPrivateStreamAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.HikDirectPrivateStreamAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11296]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.HikGenericPsNotShadowed]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.HikGenericPsNotShadowed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.HikGenericPsNotShadowed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11323]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.HikOneGiBCapIsTheRowCap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.HikOneGiBCapIsTheRowCap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.HikOneGiBCapIsTheRowCap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11356]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.HikDiscriminatorIsPrivateStream2]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.HikDiscriminatorIsPrivateStream2]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.HikDiscriminatorIsPrivateStream2]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11378]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.HikDirectPathAcceptsBcOnly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.HikDirectPathAcceptsBcOnly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.HikDirectPathAcceptsBcOnly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11439]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MpegPsPesLengthsAreBigEndian]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.MpegPsPesLengthsAreBigEndian]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MpegPsPesLengthsAreBigEndian]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11481]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.OomWindowBufferReturnsCode]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.OomWindowBufferReturnsCode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.OomWindowBufferReturnsCode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11563]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.OomTailBufferReturnsCode]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.OomTailBufferReturnsCode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.OomTailBufferReturnsCode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11584]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.OomNoExceptionEscapesEngine]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.OomNoExceptionEscapesEngine]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.OomNoExceptionEscapesEngine]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11613]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.AepRifxAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.AepRifxAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.AepRifxAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11659]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.AepXmpEndExtent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.AepXmpEndExtent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.AepXmpEndExtent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11690]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.AepBeLengthByteSwap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.AepBeLengthByteSwap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.AepBeLengthByteSwap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11724]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.AepCapThirtyMiB]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.AepCapThirtyMiB]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.AepCapThirtyMiB]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11753]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.OneChunkChainWalksPastTheStandInCap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.OneChunkChainWalksPastTheStandInCap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.OneChunkChainWalksPastTheStandInCap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11812]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.OneFirstChunkZeroSizePublishesTwoDeviceFlat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.OneFirstChunkZeroSizePublishesTwoDeviceFlat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.OneFirstChunkZeroSizePublishesTwoDeviceFlat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11842]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.OneMidChainOversizeChunkPublishesTwoDeviceFlat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.OneMidChainOversizeChunkPublishesTwoDeviceFlat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.OneMidChainOversizeChunkPublishesTwoDeviceFlat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11877]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.OneChunkChainHostileVariantsSalvageAndRefuse]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.OneChunkChainHostileVariantsSalvageAndRefuse]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.OneChunkChainHostileVariantsSalvageAndRefuse]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:11911]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.ThreeDsSectionWalkUsesTheStrideFields]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.ThreeDsSectionWalkUsesTheStrideFields]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.ThreeDsSectionWalkUsesTheStrideFields]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12006]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.X3fSectionDirectoryWalkCarvesTrueEnd]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.X3fSectionDirectoryWalkCarvesTrueEnd]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.X3fSectionDirectoryWalkCarvesTrueEnd]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12324]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.X3fTornSalvagesTheWalkedPrefix]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Carve.X3fTornSalvagesTheWalkedPrefix]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.X3fTornSalvagesTheWalkedPrefix]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12370]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawSigns.UnitsAreKib]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawSigns.UnitsAreKib]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawSigns.UnitsAreKib]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:996]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawSigns.CarvesFiveMiBVob]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawSigns.CarvesFiveMiBVob]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawSigns.CarvesFiveMiBVob]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1103]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawSigns.MinFloorOneKib]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawSigns.MinFloorOneKib]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawSigns.MinFloorOneKib]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1132]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawSigns.TailPairWindowScaled]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawSigns.TailPairWindowScaled]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawSigns.TailPairWindowScaled]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1153]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.ParseTheOgsShape]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.ParseTheOgsShape]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.ParseTheOgsShape]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:7899]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.ParseOgShippedExtremes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.ParseOgShippedExtremes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.ParseOgShippedExtremes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8790]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.ManifestEightCharExtParses]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.ManifestEightCharExtParses]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.ManifestEightCharExtParses]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8852]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.HostileBlobStillAtomic]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.HostileBlobStillAtomic]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.HostileBlobStillAtomic]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8884]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.RegistrationRoundTripProceedsToScan]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.RegistrationRoundTripProceedsToScan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.RegistrationRoundTripProceedsToScan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8909]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.ShortFragmentMatchesInCore]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.ShortFragmentMatchesInCore]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.ShortFragmentMatchesInCore]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8958]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.DefaultSetShipsAndLoads]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.DefaultSetShipsAndLoads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.DefaultSetShipsAndLoads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9003]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.OgAbsentFromStockExtensionsCarried]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.OgAbsentFromStockExtensionsCarried]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.OgAbsentFromStockExtensionsCarried]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:9029]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.LowercaseHexSignLiveAndRegistered]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.LowercaseHexSignLiveAndRegistered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.LowercaseHexSignLiveAndRegistered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12064]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.UppercaseHexSignMatches]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.UppercaseHexSignMatches]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.UppercaseHexSignMatches]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12082]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.DefaultSetEffectiveCount]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.DefaultSetEffectiveCount]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.DefaultSetEffectiveCount]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12094]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.UnknownMemberIgnored]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.UnknownMemberIgnored]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.UnknownMemberIgnored]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12127]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.MemberNamesCaseInsensitive]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.MemberNamesCaseInsensitive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.MemberNamesCaseInsensitive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12142]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.MalformedSignSkippedNotFatal]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.MalformedSignSkippedNotFatal]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.MalformedSignSkippedNotFatal]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12153]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.OddHexSkipped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.OddHexSkipped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.OddHexSkipped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12165]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.AllMalformedSignsLoadStructurally]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.AllMalformedSignsLoadStructurally]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.AllMalformedSignsLoadStructurally]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12176]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.TornFileStillRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.TornFileStillRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.TornFileStillRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12188]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.ShippedOgFileVerbatimLoads]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.ShippedOgFileVerbatimLoads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.ShippedOgFileVerbatimLoads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12202]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.DuplicateKeySemantics]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.DuplicateKeySemantics]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.DuplicateKeySemantics]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12247]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[UserSigns.ShippedCensusDedupTruth]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=UserSigns.ShippedCensusDedupTruth]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[UserSigns.ShippedCensusDedupTruth]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12295]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveStats.UserSignFilesCountInCarvedFound]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveStats.UserSignFilesCountInCarvedFound]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveStats.UserSignFilesCountInCarvedFound]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:8755]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveStats.StockRawSignFilesCountInCarvedFound]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveStats.StockRawSignFilesCountInCarvedFound]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveStats.StockRawSignFilesCountInCarvedFound]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:12221]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveStats.UserSignCarvedFoundSessionRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=CarveStats.UserSignCarvedFoundSessionRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveStats.UserSignCarvedFoundSessionRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:365]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ConfigDisplay.SummaryFamilyUnificationGroups]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ConfigDisplay.SummaryFamilyUnificationGroups]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ConfigDisplay.SummaryFamilyUnificationGroups]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:14]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ConfigDisplay.SummaryFamilyUngroupedStandsAlone]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ConfigDisplay.SummaryFamilyUngroupedStandsAlone]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ConfigDisplay.SummaryFamilyUngroupedStandsAlone]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:47]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ConfigDisplay.SummaryFamilyNullAndEmptySafe]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ConfigDisplay.SummaryFamilyNullAndEmptySafe]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ConfigDisplay.SummaryFamilyNullAndEmptySafe]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:55]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ConfigDisplay.TypeDisplayOrderVerbatim]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ConfigDisplay.TypeDisplayOrderVerbatim]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ConfigDisplay.TypeDisplayOrderVerbatim]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:60]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ConfigDisplay.TypeDisplayOrderUnrankedReturnsFalse]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ConfigDisplay.TypeDisplayOrderUnrankedReturnsFalse]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ConfigDisplay.TypeDisplayOrderUnrankedReturnsFalse]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:94]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ConfigDisplay.TypeDisplayCategoryNamesVerbatim]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ConfigDisplay.TypeDisplayCategoryNamesVerbatim]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ConfigDisplay.TypeDisplayCategoryNamesVerbatim]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:105]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ProgressDisplay.RemapCurveTable]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ProgressDisplay.RemapCurveTable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ProgressDisplay.RemapCurveTable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:113]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ProgressDisplay.StallCapIsTheDefensiveBranch]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ProgressDisplay.StallCapIsTheDefensiveBranch]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ProgressDisplay.StallCapIsTheDefensiveBranch]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:138]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ProgressDisplay.PercentTypeDispatch]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ProgressDisplay.PercentTypeDispatch]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ProgressDisplay.PercentTypeDispatch]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:151]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ProgressDisplay.MonotonicPublishGuard]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ProgressDisplay.MonotonicPublishGuard]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ProgressDisplay.MonotonicPublishGuard]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:167]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ProgressDisplay.CorePermilleStaysLinear]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ProgressDisplay.CorePermilleStaysLinear]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ProgressDisplay.CorePermilleStaysLinear]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:175]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Eta.WholeRunModel]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Eta.WholeRunModel]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Eta.WholeRunModel]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:196]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Eta.EmaDamping]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Eta.EmaDamping]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Eta.EmaDamping]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:211]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Eta.TimeBudgetVariant]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Eta.TimeBudgetVariant]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Eta.TimeBudgetVariant]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:235]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Eta.PlaceholderAndCountdownFormat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Eta.PlaceholderAndCountdownFormat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Eta.PlaceholderAndCountdownFormat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:249]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Eta.ParameterErrorClass]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Eta.ParameterErrorClass]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Eta.ParameterErrorClass]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_config_display.cpp:265]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[VolumeLabel.Ext4FromSuperblock]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=VolumeLabel.Ext4FromSuperblock]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[VolumeLabel.Ext4FromSuperblock]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_vollabel.cpp:62]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[VolumeLabel.Ext4LabellessYieldsEmpty]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=VolumeLabel.Ext4LabellessYieldsEmpty]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[VolumeLabel.Ext4LabellessYieldsEmpty]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_vollabel.cpp:78]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[VolumeLabel.FatFromBootSector]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=VolumeLabel.FatFromBootSector]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[VolumeLabel.FatFromBootSector]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_vollabel.cpp:91]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[VolumeLabel.FatLabellessYieldsEmpty]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=VolumeLabel.FatLabellessYieldsEmpty]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[VolumeLabel.FatLabellessYieldsEmpty]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_vollabel.cpp:107]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[VolumeLabel.NtfsFromVolumeRecord]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=VolumeLabel.NtfsFromVolumeRecord]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[VolumeLabel.NtfsFromVolumeRecord]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_vollabel.cpp:117]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[VolumeLabel.CallerValueWins]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=VolumeLabel.CallerValueWins]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[VolumeLabel.CallerValueWins]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_vollabel.cpp:226]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[VolumeLabel.UnknownFamilyLeavesFallback]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=VolumeLabel.UnknownFamilyLeavesFallback]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[VolumeLabel.UnknownFamilyLeavesFallback]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_vollabel.cpp:257]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.RoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Crsf.RoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.RoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:56]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.ExtentLogicalRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Crsf.ExtentLogicalRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.ExtentLogicalRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:100]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.CorruptionSkipsForward]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Crsf.CorruptionSkipsForward]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.CorruptionSkipsForward]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:151]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.TreeRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Crsf.TreeRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.TreeRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:179]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.TinyChunkDeflatesWhenSmaller]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Crsf.TinyChunkDeflatesWhenSmaller]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.TinyChunkDeflatesWhenSmaller]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:238]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.ReconstructedCarveTreeRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Crsf.ReconstructedCarveTreeRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.ReconstructedCarveTreeRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:263]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.VolumeInfoIdentityCardRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Crsf.VolumeInfoIdentityCardRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.VolumeInfoIdentityCardRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:338]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.TornCheckpointSalvagesPrefix]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Crsf.TornCheckpointSalvagesPrefix]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.TornCheckpointSalvagesPrefix]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:390]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.PeekRecordHeaderOnly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Crsf.PeekRecordHeaderOnly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.PeekRecordHeaderOnly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1630]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.LostDirFoldersRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Crsf.LostDirFoldersRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.LostDirFoldersRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1887]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SessionTree.RefreshStatsIsRecursive]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=SessionTree.RefreshStatsIsRecursive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SessionTree.RefreshStatsIsRecursive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:493]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SessionTree.ReloadedRootStatsAreRecursive]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=SessionTree.ReloadedRootStatsAreRecursive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SessionTree.ReloadedRootStatsAreRecursive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:507]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SessionTree.StatsDeletedCountRecursive]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=SessionTree.StatsDeletedCountRecursive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SessionTree.StatsDeletedCountRecursive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:535]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.GradeRoleAndProvenanceRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.GradeRoleAndProvenanceRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.GradeRoleAndProvenanceRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:640]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.OldFormatStillLoads]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.OldFormatStillLoads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.OldFormatStillLoads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:685]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.ForwardParentDeferredAndCounted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.ForwardParentDeferredAndCounted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.ForwardParentDeferredAndCounted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:709]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.BadMarkerFailsTheRecord]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.BadMarkerFailsTheRecord]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.BadMarkerFailsTheRecord]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:764]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.TruncatedTailFailsAfterFullCount]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.TruncatedTailFailsAfterFullCount]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.TruncatedTailFailsAfterFullCount]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:774]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.HostileNameAndRoleLengthBombs]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.HostileNameAndRoleLengthBombs]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.HostileNameAndRoleLengthBombs]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:787]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.CorruptRecordEarlyExitsReleaseProps]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.CorruptRecordEarlyExitsReleaseProps]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.CorruptRecordEarlyExitsReleaseProps]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:872]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.RawNameRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.RawNameRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.RawNameRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:996]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.OldFormatWithoutRawNameStillLoads]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.OldFormatWithoutRawNameStillLoads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.OldFormatWithoutRawNameStillLoads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1034]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.SaveEmitsProgress]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.SaveEmitsProgress]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.SaveEmitsProgress]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1066]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.SaveCancelsCleanly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.SaveCancelsCleanly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.SaveCancelsCleanly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1110]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.AutoNamingConvention]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.AutoNamingConvention]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.AutoNamingConvention]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1133]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.BtrfsExtrasRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.BtrfsExtrasRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.BtrfsExtrasRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1162]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.ZfsSecondaryFlagsRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.ZfsSecondaryFlagsRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.ZfsSecondaryFlagsRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1197]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.EngineExtrasAbsentWhenUnpopulated]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.EngineExtrasAbsentWhenUnpopulated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.EngineExtrasAbsentWhenUnpopulated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1238]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.FatOriginClusterRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.FatOriginClusterRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.FatOriginClusterRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1268]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.NtfsOriginLcnRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.NtfsOriginLcnRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.NtfsOriginLcnRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1309]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.NtfsTotalClustersRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.NtfsTotalClustersRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.NtfsTotalClustersRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1365]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.NtfsParentRefRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.NtfsParentRefRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.NtfsParentRefRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1434]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.FatParentClusterRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.FatParentClusterRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.FatParentClusterRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1493]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.IdentityCardFirmwareWwnInterface]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.IdentityCardFirmwareWwnInterface]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.IdentityCardFirmwareWwnInterface]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1541]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.BootSnapshotRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.BootSnapshotRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.BootSnapshotRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1565]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.SaveUnixVTailRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.SaveUnixVTailRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.SaveUnixVTailRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1592]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.DeserializeProgressPerNodeAndCancel]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.DeserializeProgressPerNodeAndCancel]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.DeserializeProgressPerNodeAndCancel]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1780]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.LoadSessionSummaryPropagatesProgressAndCancel]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.LoadSessionSummaryPropagatesProgressAndCancel]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.LoadSessionSummaryPropagatesProgressAndCancel]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1836]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.SizeExceedsBlocksFlagRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Session.SizeExceedsBlocksFlagRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.SizeExceedsBlocksFlagRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1939]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SessionSummary.ReadSummaryLightRecordsOnly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=SessionSummary.ReadSummaryLightRecordsOnly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SessionSummary.ReadSummaryLightRecordsOnly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1676]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SessionSummary.LegacySessionWithoutVolumeInfo]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=SessionSummary.LegacySessionWithoutVolumeInfo]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SessionSummary.LegacySessionWithoutVolumeInfo]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1733]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.DynamicDiskVolumeScan]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.DynamicDiskVolumeScan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.DynamicDiskVolumeScan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:173]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.WholeDiskScansTableVolumes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.WholeDiskScansTableVolumes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.WholeDiskScansTableVolumes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:256]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.WholeDiskGptTwin]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.WholeDiskGptTwin]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.WholeDiskGptTwin]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:280]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.WholeDiskMixedFamilies]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.WholeDiskMixedFamilies]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.WholeDiskMixedFamilies]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:302]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.WholeDiskFsMarkerNamesForeignVolume]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.WholeDiskFsMarkerNamesForeignVolume]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.WholeDiskFsMarkerNamesForeignVolume]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:346]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.WholeDiskDamagedVolumeSkippedHonestly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.WholeDiskDamagedVolumeSkippedHonestly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.WholeDiskDamagedVolumeSkippedHonestly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:397]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.WholeDiskSuperfloppyUnchanged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.WholeDiskSuperfloppyUnchanged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.WholeDiskSuperfloppyUnchanged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:419]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.WholeDiskNoDoubleScan]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.WholeDiskNoDoubleScan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.WholeDiskNoDoubleScan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:445]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.WholeDiskVolumeRootsCarryDisplayNames]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.WholeDiskVolumeRootsCarryDisplayNames]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.WholeDiskVolumeRootsCarryDisplayNames]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:471]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.WholeDiskKnownVsLostDistinguishable]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.WholeDiskKnownVsLostDistinguishable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.WholeDiskKnownVsLostDistinguishable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:507]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.LostPartUnnamedCandidateNamedLostPartitionN]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.LostPartUnnamedCandidateNamedLostPartitionN]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.LostPartUnnamedCandidateNamedLostPartitionN]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:543]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.LostPartNamedCandidateKeepsVolumeLabel]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.LostPartNamedCandidateKeepsVolumeLabel]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.LostPartNamedCandidateKeepsVolumeLabel]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:574]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.LostPartInsertionCapTenTargets]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.LostPartInsertionCapTenTargets]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.LostPartInsertionCapTenTargets]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:626]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.LostPartCandidateListingCarriesLayoutFields]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.LostPartCandidateListingCarriesLayoutFields]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.LostPartCandidateListingCarriesLayoutFields]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:656]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.WholeDiskVolumeRootNamesRoundTripSession]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.WholeDiskVolumeRootNamesRoundTripSession]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.WholeDiskVolumeRootNamesRoundTripSession]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:683]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.JunkFilteredFromResults]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.JunkFilteredFromResults]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.JunkFilteredFromResults]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:806]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.LogSilentByDefault]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.LogSilentByDefault]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.LogSilentByDefault]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1015]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.LogSinkReceivesScanBrackets]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.LogSinkReceivesScanBrackets]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.LogSinkReceivesScanBrackets]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1035]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.MemoryMonitorEmitsUnderPressure]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.MemoryMonitorEmitsUnderPressure]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.MemoryMonitorEmitsUnderPressure]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1076]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.WeightedProgress]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.WeightedProgress]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.WeightedProgress]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1107]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.ProbeFamily]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.ProbeFamily]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.ProbeFamily]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1117]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.SpoofedExfatOemRejectedBeforeEngine]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.SpoofedExfatOemRejectedBeforeEngine]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.SpoofedExfatOemRejectedBeforeEngine]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1147]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.SpoofedFatBootUnreadableRootRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.SpoofedFatBootUnreadableRootRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.SpoofedFatBootUnreadableRootRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1160]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.RealExfatBootContentVerified]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.RealExfatBootContentVerified]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.RealExfatBootContentVerified]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1173]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.FsFamilyValuesMatchContract]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.FsFamilyValuesMatchContract]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.FsFamilyValuesMatchContract]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1232]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.UdfProbeFailsOnMkudffsShapeLikeTheOg]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.UdfProbeFailsOnMkudffsShapeLikeTheOg]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.UdfProbeFailsOnMkudffsShapeLikeTheOg]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1249]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.ProbesUdfVolume]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.ProbesUdfVolume]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.ProbesUdfVolume]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1297]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.UdfProbeRequiresBothDescriptorReads]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.UdfProbeRequiresBothDescriptorReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.UdfProbeRequiresBothDescriptorReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1305]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.UdfNotConfusedWithFat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.UdfNotConfusedWithFat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.UdfNotConfusedWithFat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1319]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.UdfSurfacesAsCarveFamily]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.UdfSurfacesAsCarveFamily]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.UdfSurfacesAsCarveFamily]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1343]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.CarveSingleNamingFamily]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.CarveSingleNamingFamily]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.CarveSingleNamingFamily]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1467]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.CarveTallySessionRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.CarveTallySessionRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.CarveTallySessionRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1596]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.CarveVolumeGroupTreeShape]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.CarveVolumeGroupTreeShape]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.CarveVolumeGroupTreeShape]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1758]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.CarveVolumeGroupLazyNoCarvesNoChild]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.CarveVolumeGroupLazyNoCarvesNoChild]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.CarveVolumeGroupLazyNoCarvesNoChild]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1871]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.CarveVolumeGroupNameForms]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.CarveVolumeGroupNameForms]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.CarveVolumeGroupNameForms]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1909]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.StagedRunAndSessionRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.StagedRunAndSessionRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.StagedRunAndSessionRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1986]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.PreScanRepairRestoresTornFatAndScans]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.PreScanRepairRestoresTornFatAndScans]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.PreScanRepairRestoresTornFatAndScans]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2315]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.PreScanRepairDeniedNeverWrites]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.PreScanRepairDeniedNeverWrites]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.PreScanRepairDeniedNeverWrites]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2367]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.PreScanRepairWithoutHooksNeverWrites]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.PreScanRepairWithoutHooksNeverWrites]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.PreScanRepairWithoutHooksNeverWrites]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2399]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.ReadPipeBuildsCacheOverRetryForValidGeometry]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.ReadPipeBuildsCacheOverRetryForValidGeometry]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.ReadPipeBuildsCacheOverRetryForValidGeometry]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2758]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.ReadPipeDegradesToBareSliceWhenCacheCreationFails]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.ReadPipeDegradesToBareSliceWhenCacheCreationFails]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.ReadPipeDegradesToBareSliceWhenCacheCreationFails]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2773]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.ReadPipeZeroCapacityDegradesToBareSlice]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.ReadPipeZeroCapacityDegradesToBareSlice]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.ReadPipeZeroCapacityDegradesToBareSlice]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2790]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.LogSinkRegistrySurvivesHeavyChurn]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.LogSinkRegistrySurvivesHeavyChurn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.LogSinkRegistrySurvivesHeavyChurn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2800]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.IdentityCardV4SnapshotAndAxes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.IdentityCardV4SnapshotAndAxes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.IdentityCardV4SnapshotAndAxes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2829]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.DynamicDiskScanStampsBasisAndSessionRoundTrips]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.DynamicDiskScanStampsBasisAndSessionRoundTrips]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.DynamicDiskScanStampsBasisAndSessionRoundTrips]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2906]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.LostRootNamedFilePathLost]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.LostRootNamedFilePathLost]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.LostRootNamedFilePathLost]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:3015]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.TelemetryCountersMoveDuringCarveStage]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.TelemetryCountersMoveDuringCarveStage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.TelemetryCountersMoveDuringCarveStage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:3045]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.SpannedLdmTransientMemberFaultScans]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.SpannedLdmTransientMemberFaultScans]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.SpannedLdmTransientMemberFaultScans]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:3185]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.LdmMemberResolversConstructThroughBuildReadPipe]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Orchestrator.LdmMemberResolversConstructThroughBuildReadPipe]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.LdmMemberResolversConstructThroughBuildReadPipe]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:3302]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FilterFolder.CorpusListsPinned]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FilterFolder.CorpusListsPinned]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FilterFolder.CorpusListsPinned]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:743]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FilterFolder.JunkGateFixtureFileScansAndFlagsHiddenSet]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FilterFolder.JunkGateFixtureFileScansAndFlagsHiddenSet]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FilterFolder.JunkGateFixtureFileScansAndFlagsHiddenSet]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:935]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SafeWriter.OverlapRefusal]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=SafeWriter.OverlapRefusal]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SafeWriter.OverlapRefusal]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2069]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SafeWriter.ExplicitSelectionStillRecoversJunkFlaggedNode]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=SafeWriter.ExplicitSelectionStillRecoversJunkFlaggedNode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SafeWriter.ExplicitSelectionStillRecoversJunkFlaggedNode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2122]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SafeWriter.CollisionAndSanitize]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=SafeWriter.CollisionAndSanitize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SafeWriter.CollisionAndSanitize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2163]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SafeWriter.ResidentPrefixNotOverwritten]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=SafeWriter.ResidentPrefixNotOverwritten]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SafeWriter.ResidentPrefixNotOverwritten]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2179]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SafeWriter.InvalidUtf8NameRestoredFromRaw]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=SafeWriter.InvalidUtf8NameRestoredFromRaw]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SafeWriter.InvalidUtf8NameRestoredFromRaw]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2708]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanEvents.ErrorRangeGoesToLiveSink]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ScanEvents.ErrorRangeGoesToLiveSink]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanEvents.ErrorRangeGoesToLiveSink]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2492]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanEvents.SinkSwitchDeliversAllTypes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ScanEvents.SinkSwitchDeliversAllTypes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanEvents.SinkSwitchDeliversAllTypes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2520]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanEvents.SingleBoundaryAtTeardown]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ScanEvents.SingleBoundaryAtTeardown]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanEvents.SingleBoundaryAtTeardown]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2549]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanEvents.ErrorRangeCarriesModuleOperationAttempts]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ScanEvents.ErrorRangeCarriesModuleOperationAttempts]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanEvents.ErrorRangeCarriesModuleOperationAttempts]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2618]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanEvents.ProbeBlockFailureAttributesToOrchestrator]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ScanEvents.ProbeBlockFailureAttributesToOrchestrator]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanEvents.ProbeBlockFailureAttributesToOrchestrator]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2642]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanEvents.CarveErrorRangeAttributesToCarveModule]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ScanEvents.CarveErrorRangeAttributesToCarveModule]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanEvents.CarveErrorRangeAttributesToCarveModule]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2666]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanConfig.DeepStageDefaultsOnAndWholeDiskOff]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ScanConfig.DeepStageDefaultsOnAndWholeDiskOff]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanConfig.DeepStageDefaultsOnAndWholeDiskOff]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:2688]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Transport.PeGateDecisionMatchesOgProbe]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Transport.PeGateDecisionMatchesOgProbe]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Transport.PeGateDecisionMatchesOgProbe]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_transport_logic.cpp:23]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Transport.QuiesceChainConstantsMatchCorpus]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Transport.QuiesceChainConstantsMatchCorpus]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Transport.QuiesceChainConstantsMatchCorpus]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_transport_logic.cpp:40]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Transport.EnumerateBoundMatchesDriverGrammarSpace]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Transport.EnumerateBoundMatchesDriverGrammarSpace]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Transport.EnumerateBoundMatchesDriverGrammarSpace]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_transport_logic.cpp:56]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Transport.DisconnectClassifiedDistinctly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Transport.DisconnectClassifiedDistinctly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Transport.DisconnectClassifiedDistinctly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_transport_logic.cpp:65]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Transport.MediaErrorsStillReadFailed]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Transport.MediaErrorsStillReadFailed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Transport.MediaErrorsStillReadFailed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_transport_logic.cpp:78]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DriverStatus.ReadyWhenAllFacetsGreen]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DriverStatus.ReadyWhenAllFacetsGreen]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DriverStatus.ReadyWhenAllFacetsGreen]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_driver_status.cpp:35]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DriverStatus.AbsentWhenServiceNotInstalled]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DriverStatus.AbsentWhenServiceNotInstalled]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DriverStatus.AbsentWhenServiceNotInstalled]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_driver_status.cpp:41]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DriverStatus.NotStartedWhenInstalledButStopped]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DriverStatus.NotStartedWhenInstalledButStopped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DriverStatus.NotStartedWhenInstalledButStopped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_driver_status.cpp:53]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DriverStatus.UnauthorizedWhenOpenDeniedByDacl]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DriverStatus.UnauthorizedWhenOpenDeniedByDacl]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DriverStatus.UnauthorizedWhenOpenDeniedByDacl]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_driver_status.cpp:64]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DriverStatus.ErroredWhenOpenFailsWithAnyOtherGle]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DriverStatus.ErroredWhenOpenFailsWithAnyOtherGle]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DriverStatus.ErroredWhenOpenFailsWithAnyOtherGle]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_driver_status.cpp:77]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DriverStatus.ErroredWhenGetInfoFailsOnOpenedDevice]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DriverStatus.ErroredWhenGetInfoFailsOnOpenedDevice]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DriverStatus.ErroredWhenGetInfoFailsOnOpenedDevice]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_driver_status.cpp:91]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DriverStatus.VersionMismatchWhenMajorDiffers]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DriverStatus.VersionMismatchWhenMajorDiffers]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DriverStatus.VersionMismatchWhenMajorDiffers]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_driver_status.cpp:99]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DriverStatus.MinorOnlyDivergenceStaysReady]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DriverStatus.MinorOnlyDivergenceStaysReady]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DriverStatus.MinorOnlyDivergenceStaysReady]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_driver_status.cpp:108]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DriverStatus.ClassificationOrderIsFailFastFirstMatch]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DriverStatus.ClassificationOrderIsFailFastFirstMatch]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DriverStatus.ClassificationOrderIsFailFastFirstMatch]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_driver_status.cpp:116]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DriverStatus.StateNamesAndErrorCodesAreTotalFunctions]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DriverStatus.StateNamesAndErrorCodesAreTotalFunctions]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DriverStatus.StateNamesAndErrorCodesAreTotalFunctions]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_driver_status.cpp:137]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DriverStatus.DefaultStatusIsAbsentNotReady]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DriverStatus.DefaultStatusIsAbsentNotReady]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DriverStatus.DefaultStatusIsAbsentNotReady]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_driver_status.cpp:153]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DriverStatus.RequiredWireMajorMatchesDriverContract]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DriverStatus.RequiredWireMajorMatchesDriverContract]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DriverStatus.RequiredWireMajorMatchesDriverContract]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_driver_status.cpp:162]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanTruth.FullyUnreadableVolumeRefusesInsteadOfFakeCompleting]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ScanTruth.FullyUnreadableVolumeRefusesInsteadOfFakeCompleting]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanTruth.FullyUnreadableVolumeRefusesInsteadOfFakeCompleting]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_scan_truth.cpp:80]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanTruth.PartialDamageKeepsSalvageSemanticsAndSurfacesCoverage]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ScanTruth.PartialDamageKeepsSalvageSemanticsAndSurfacesCoverage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanTruth.PartialDamageKeepsSalvageSemanticsAndSurfacesCoverage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_scan_truth.cpp:107]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanTruth.BlindProbeIsFlaggedWhileTheRestReads]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ScanTruth.BlindProbeIsFlaggedWhileTheRestReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanTruth.BlindProbeIsFlaggedWhileTheRestReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_scan_truth.cpp:130]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanTruth.HealthyZeroVolumeStillCompletesCleanly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ScanTruth.HealthyZeroVolumeStillCompletesCleanly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanTruth.HealthyZeroVolumeStillCompletesCleanly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_scan_truth.cpp:149]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanTruth.StatsFieldsAreZeroInitialized]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ScanTruth.StatsFieldsAreZeroInitialized]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanTruth.StatsFieldsAreZeroInitialized]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_scan_truth.cpp:166]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.TokenCheckpointBlocksAndResumes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PauseResume.TokenCheckpointBlocksAndResumes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.TokenCheckpointBlocksAndResumes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:187]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.CancelWhilePausedWakesWaiter]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PauseResume.CancelWhilePausedWakesWaiter]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.CancelWhilePausedWakesWaiter]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:210]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.PausedMsAccounting]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PauseResume.PausedMsAccounting]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.PausedMsAccounting]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:226]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.ScanPauseFreezesAndElapsedExcludesPause]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PauseResume.ScanPauseFreezesAndElapsedExcludesPause]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.ScanPauseFreezesAndElapsedExcludesPause]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:241]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.SaveOnCancelStampsTruthfulMaskAndResumeSkipsQuick]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PauseResume.SaveOnCancelStampsTruthfulMaskAndResumeSkipsQuick]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.SaveOnCancelStampsTruthfulMaskAndResumeSkipsQuick]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:279]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.MaskIntegrityViolationRefusesResume]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PauseResume.MaskIntegrityViolationRefusesResume]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.MaskIntegrityViolationRefusesResume]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:481]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.ResumedSessionPreservesOriginalConfiguredMask]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PauseResume.ResumedSessionPreservesOriginalConfiguredMask]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.ResumedSessionPreservesOriginalConfiguredMask]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:552]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.SessionRestoresVolumeInfoAndCandidates]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PauseResume.SessionRestoresVolumeInfoAndCandidates]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.SessionRestoresVolumeInfoAndCandidates]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:629]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.SaveAfterResumeReloadsCleanly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PauseResume.SaveAfterResumeReloadsCleanly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.SaveAfterResumeReloadsCleanly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:985]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanStats.BytesFoundAggregates]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ScanStats.BytesFoundAggregates]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanStats.BytesFoundAggregates]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:418]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanStats.BytesFoundSessionRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ScanStats.BytesFoundSessionRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanStats.BytesFoundSessionRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:441]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Checkpoint.StageBoundaryCheckpointResumesAfterCrash]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Checkpoint.StageBoundaryCheckpointResumesAfterCrash]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Checkpoint.StageBoundaryCheckpointResumesAfterCrash]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:722]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Checkpoint.CorruptMagicRefused]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Checkpoint.CorruptMagicRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Checkpoint.CorruptMagicRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:788]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Checkpoint.CommitHookFiresAfterEveryFinalizeAndAbortsOnError]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Checkpoint.CommitHookFiresAfterEveryFinalizeAndAbortsOnError]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Checkpoint.CommitHookFiresAfterEveryFinalizeAndAbortsOnError]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:1068]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExtentBasis.TreeRebaseShiftsOnceAndIsIdempotent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExtentBasis.TreeRebaseShiftsOnceAndIsIdempotent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExtentBasis.TreeRebaseShiftsOnceAndIsIdempotent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:840]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExtentBasis.SliceScanSavesAbsoluteExtents]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExtentBasis.SliceScanSavesAbsoluteExtents]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExtentBasis.SliceScanSavesAbsoluteExtents]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:870]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExtentBasis.LegacySliceRelativeSessionNormalizedOnLoad]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExtentBasis.LegacySliceRelativeSessionNormalizedOnLoad]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExtentBasis.LegacySliceRelativeSessionNormalizedOnLoad]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:923]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Telemetry.UniformProgressAcrossStagesMonotonic]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Telemetry.UniformProgressAcrossStagesMonotonic]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Telemetry.UniformProgressAcrossStagesMonotonic]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_telemetry.cpp:214]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Telemetry.FatWalkEmitsIncrementalTicks]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Telemetry.FatWalkEmitsIncrementalTicks]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Telemetry.FatWalkEmitsIncrementalTicks]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_telemetry.cpp:249]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Telemetry.PerStageShapeAndCompletion]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Telemetry.PerStageShapeAndCompletion]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Telemetry.PerStageShapeAndCompletion]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_telemetry.cpp:294]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Telemetry.LiveErrorMapVisibleMidScan]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Telemetry.LiveErrorMapVisibleMidScan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Telemetry.LiveErrorMapVisibleMidScan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_telemetry.cpp:321]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Telemetry.QueryDuringScanIsThreadSafe]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Telemetry.QueryDuringScanIsThreadSafe]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Telemetry.QueryDuringScanIsThreadSafe]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_telemetry.cpp:369]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Telemetry.LiveAttachCounterTracksTheTree]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Telemetry.LiveAttachCounterTracksTheTree]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Telemetry.LiveAttachCounterTracksTheTree]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_telemetry.cpp:398]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Telemetry.CountersMoveDuringStage]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Telemetry.CountersMoveDuringStage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Telemetry.CountersMoveDuringStage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_telemetry.cpp:417]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.RestoresTimestampsAndAttributes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterDirs.RestoresTimestampsAndAttributes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.RestoresTimestampsAndAttributes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:328]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.ZeroTimeSlotsPassAsUnset]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterDirs.ZeroTimeSlotsPassAsUnset]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.ZeroTimeSlotsPassAsUnset]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:370]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.MetadataFailureIsNonFatal]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterDirs.MetadataFailureIsNonFatal]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.MetadataFailureIsNonFatal]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:401]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.MetadatalessBackendReportsUnsupported]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterDirs.MetadatalessBackendReportsUnsupported]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.MetadatalessBackendReportsUnsupported]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:442]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.InteriorHoleZeroFilledFullSize]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterDirs.InteriorHoleZeroFilledFullSize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.InteriorHoleZeroFilledFullSize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:481]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.LegacyCumulativeExtentsUnchanged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterDirs.LegacyCumulativeExtentsUnchanged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.LegacyCumulativeExtentsUnchanged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:561]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.StructureReconstructionWithEmptyLeaves]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterDirs.StructureReconstructionWithEmptyLeaves]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.StructureReconstructionWithEmptyLeaves]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:600]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.DeepDestChainCreatedAndIdempotentRerun]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterDirs.DeepDestChainCreatedAndIdempotentRerun]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.DeepDestChainCreatedAndIdempotentRerun]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:635]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.IntraQueueDuplicatesRenameUnderSkip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterDirs.IntraQueueDuplicatesRenameUnderSkip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.IntraQueueDuplicatesRenameUnderSkip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:683]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.PerItemErrorCollectedQueueContinues]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterDirs.PerItemErrorCollectedQueueContinues]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.PerItemErrorCollectedQueueContinues]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:745]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.MetadataOnlyItemSkipsNotZeroFills]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterDirs.MetadataOnlyItemSkipsNotZeroFills]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.MetadataOnlyItemSkipsNotZeroFills]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:784]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.ProgressExcludesErrorItems]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterDirs.ProgressExcludesErrorItems]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.ProgressExcludesErrorItems]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:847]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.CollisionNameUnderscoreFormat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterDirs.CollisionNameUnderscoreFormat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.CollisionNameUnderscoreFormat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:886]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.CollisionExhaustionFailsTheItem]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterDirs.CollisionExhaustionFailsTheItem]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.CollisionExhaustionFailsTheItem]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:939]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.MkdirSemanticsMatchShCreateDirectoryEx]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterDirs.MkdirSemanticsMatchShCreateDirectoryEx]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.MkdirSemanticsMatchShCreateDirectoryEx]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:982]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterReadPipe.TransientFailureRetriedNotZeroFilled]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterReadPipe.TransientFailureRetriedNotZeroFilled]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterReadPipe.TransientFailureRetriedNotZeroFilled]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_readpipe.cpp:198]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterReadPipe.HardFailureZeroFillsOnlyAfterRetriesSpent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterReadPipe.HardFailureZeroFillsOnlyAfterRetriesSpent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterReadPipe.HardFailureZeroFillsOnlyAfterRetriesSpent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_readpipe.cpp:252]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterReadPipe.BareDeviceLegacyShapeZeroFillsTransients]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterReadPipe.BareDeviceLegacyShapeZeroFillsTransients]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterReadPipe.BareDeviceLegacyShapeZeroFillsTransients]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_readpipe.cpp:314]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterReadPipe.ConstructionIsTheScanSubstrate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterReadPipe.ConstructionIsTheScanSubstrate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterReadPipe.ConstructionIsTheScanSubstrate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_readpipe.cpp:350]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterReadPipe.ReadErrorMarksTheLiveNode]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterReadPipe.ReadErrorMarksTheLiveNode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterReadPipe.ReadErrorMarksTheLiveNode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_readpipe.cpp:376]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterReadPipe.SettingsGateSkipsReadErrorAccounting]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=WriterReadPipe.SettingsGateSkipsReadErrorAccounting]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterReadPipe.SettingsGateSkipsReadErrorAccounting]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_readpipe.cpp:408]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LdmRecovery.SpanRecoveryReadsThroughVirtualBasis]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LdmRecovery.SpanRecoveryReadsThroughVirtualBasis]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LdmRecovery.SpanRecoveryReadsThroughVirtualBasis]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm_recovery.cpp:200]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LdmRecovery.StripeRecoveryRoundTrips]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LdmRecovery.StripeRecoveryRoundTrips]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LdmRecovery.StripeRecoveryRoundTrips]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm_recovery.cpp:263]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LdmRecovery.Raid5RecoveryWithMissingMember]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LdmRecovery.Raid5RecoveryWithMissingMember]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LdmRecovery.Raid5RecoveryWithMissingMember]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm_recovery.cpp:313]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LdmRecovery.OverlapRefusalMapsToPhysicalSpans]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LdmRecovery.OverlapRefusalMapsToPhysicalSpans]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LdmRecovery.OverlapRefusalMapsToPhysicalSpans]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm_recovery.cpp:410]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LdmRecovery.SimpleVolumeRecoveryRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LdmRecovery.SimpleVolumeRecoveryRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LdmRecovery.SimpleVolumeRecoveryRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm_recovery.cpp:483]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LdmRecovery.PreviewMatchesRecovery]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LdmRecovery.PreviewMatchesRecovery]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LdmRecovery.PreviewMatchesRecovery]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm_recovery.cpp:524]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LdmRecovery.SessionRoundTripsLayoutsAndRootBasis]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LdmRecovery.SessionRoundTripsLayoutsAndRootBasis]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LdmRecovery.SessionRoundTripsLayoutsAndRootBasis]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm_recovery.cpp:575]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Writer.ReadsRetryOnTransientFailure]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Writer.ReadsRetryOnTransientFailure]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Writer.ReadsRetryOnTransientFailure]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:755]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Writer.BareDeviceTruncatesWhatThePipeRetries]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Writer.BareDeviceTruncatesWhatThePipeRetries]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Writer.BareDeviceTruncatesWhatThePipeRetries]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:784]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Writer.RecoveryCacheServesSharedExtents]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Writer.RecoveryCacheServesSharedExtents]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Writer.RecoveryCacheServesSharedExtents]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:818]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Writer.RecoveryEntryPointsConstructThroughBuildReadPipe]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Writer.RecoveryEntryPointsConstructThroughBuildReadPipe]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Writer.RecoveryEntryPointsConstructThroughBuildReadPipe]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:845]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Writer.HardReadFailureTruncatesAndKeeps]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Writer.HardReadFailureTruncatesAndKeeps]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Writer.HardReadFailureTruncatesAndKeeps]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:863]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Writer.ReadErrorLogTriFormat]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Writer.ReadErrorLogTriFormat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Writer.ReadErrorLogTriFormat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:905]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Writer.FailureCauseTableMatchesOgRuntimeStrings]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Writer.FailureCauseTableMatchesOgRuntimeStrings]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Writer.FailureCauseTableMatchesOgRuntimeStrings]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:953]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Writer.BeyondVolumeReadRecordsCauseFourAndClampsStraddle]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Writer.BeyondVolumeReadRecordsCauseFourAndClampsStraddle]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Writer.BeyondVolumeReadRecordsCauseFourAndClampsStraddle]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:973]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Writer.ShortExtentMetadataCommitsPartial]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Writer.ShortExtentMetadataCommitsPartial]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Writer.ShortExtentMetadataCommitsPartial]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:1018]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Writer.CompressedUndecodableBlockStillSalvages]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Writer.CompressedUndecodableBlockStillSalvages]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Writer.CompressedUndecodableBlockStillSalvages]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:1054]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Reghive.ParsesValidHiveAndRejectsGarbage]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Reghive.ParsesValidHiveAndRejectsGarbage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Reghive.ParsesValidHiveAndRejectsGarbage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:210]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Reghive.RejectsCorruptedHeaderChecksum]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Reghive.RejectsCorruptedHeaderChecksum]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Reghive.RejectsCorruptedHeaderChecksum]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:224]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Reghive.WalksControlSetKeyFamilyCaseInsensitive]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Reghive.WalksControlSetKeyFamilyCaseInsensitive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Reghive.WalksControlSetKeyFamilyCaseInsensitive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:233]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Reghive.EnumeratesSubkeyNames]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Reghive.EnumeratesSubkeyNames]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Reghive.EnumeratesSubkeyNames]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:245]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Reghive.ReadsBinaryValueDataPointedAndInline]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Reghive.ReadsBinaryValueDataPointedAndInline]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Reghive.ReadsBinaryValueDataPointedAndInline]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:259]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Reghive.UnicodeKeyNamesWalkAsAscii]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Reghive.UnicodeKeyNamesWalkAsAscii]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Reghive.UnicodeKeyNamesWalkAsAscii]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:295]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Reghive.TruncatedAndMutatedHivesParseOrRefuseWithoutCrashing]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Reghive.TruncatedAndMutatedHivesParseOrRefuseWithoutCrashing]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Reghive.TruncatedAndMutatedHivesParseOrRefuseWithoutCrashing]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:410]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FveAutoUnlock.HarvestsPerGuidDataBlobs]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FveAutoUnlock.HarvestsPerGuidDataBlobs]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FveAutoUnlock.HarvestsPerGuidDataBlobs]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:311]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FveAutoUnlock.HarvestCapsBlobsAt256Bytes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FveAutoUnlock.HarvestCapsBlobsAt256Bytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FveAutoUnlock.HarvestCapsBlobsAt256Bytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:332]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FveAutoUnlock.HarvestCapsRecordsAt100AndSkipsTinyBlobs]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FveAutoUnlock.HarvestCapsRecordsAt100AndSkipsTinyBlobs]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FveAutoUnlock.HarvestCapsRecordsAt100AndSkipsTinyBlobs]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:350]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FveAutoUnlock.SelectionPrefersMatchingVolumeGuid]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FveAutoUnlock.SelectionPrefersMatchingVolumeGuid]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FveAutoUnlock.SelectionPrefersMatchingVolumeGuid]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:372]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OfflineOs.ProfileReadsCurrentVersion]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=OfflineOs.ProfileReadsCurrentVersion]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OfflineOs.ProfileReadsCurrentVersion]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:496]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OfflineOs.ControlSetResolvesViaSelectWithFallback]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=OfflineOs.ControlSetResolvesViaSelectWithFallback]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OfflineOs.ControlSetResolvesViaSelectWithFallback]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:526]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OfflineOs.SystemPartitionFromMountedDevices]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=OfflineOs.SystemPartitionFromMountedDevices]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OfflineOs.SystemPartitionFromMountedDevices]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:548]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OfflineOs.ServerVsClientClassification]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=OfflineOs.ServerVsClientClassification]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OfflineOs.ServerVsClientClassification]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:572]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[OfflineOs.ToleratesMissingKeysAndOddValueShapes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=OfflineOs.ToleratesMissingKeysAndOddValueShapes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[OfflineOs.ToleratesMissingKeysAndOddValueShapes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:602]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BcdFix.BcdFindsBootPartitionBySignature]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=BcdFix.BcdFindsBootPartitionBySignature]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BcdFix.BcdFindsBootPartitionBySignature]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bcdfix.cpp:71]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BcdFix.BcdOffsetRewriteWritesNewLba]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=BcdFix.BcdOffsetRewriteWritesNewLba]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BcdFix.BcdOffsetRewriteWritesNewLba]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bcdfix.cpp:94]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BcdFix.BcdUndoRestoresOffsets]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=BcdFix.BcdUndoRestoresOffsets]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BcdFix.BcdUndoRestoresOffsets]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bcdfix.cpp:117]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BcdFix.BootfixRefusedWithoutConsent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=BcdFix.BootfixRefusedWithoutConsent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BcdFix.BootfixRefusedWithoutConsent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bcdfix.cpp:138]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BcdFix.BoundariesNeverPlan]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=BcdFix.BoundariesNeverPlan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BcdFix.BoundariesNeverPlan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bcdfix.cpp:161]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvramFix.MatchesMbrBySignatureStartSize]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvramFix.MatchesMbrBySignatureStartSize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvramFix.MatchesMbrBySignatureStartSize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvramfix.cpp:161]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvramFix.MatchesGptByGuidAlone]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvramFix.MatchesGptByGuidAlone]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvramFix.MatchesGptByGuidAlone]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvramfix.cpp:185]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvramFix.MbrFlagsMayRideDifferentNodes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvramFix.MbrFlagsMayRideDifferentNodes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvramFix.MbrFlagsMayRideDifferentNodes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvramfix.cpp:225]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvramFix.PatchUpdatesLoadOption]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvramFix.PatchUpdatesLoadOption]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvramFix.PatchUpdatesLoadOption]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvramfix.cpp:249]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvramFix.UnmatchedEntriesUntouched]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvramFix.UnmatchedEntriesUntouched]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvramFix.UnmatchedEntriesUntouched]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvramfix.cpp:285]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvramFix.ConsentDefaultDeny]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvramFix.ConsentDefaultDeny]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvramFix.ConsentDefaultDeny]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvramfix.cpp:301]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvramFix.UndoRestoresOriginalVariables]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvramFix.UndoRestoresOriginalVariables]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvramFix.UndoRestoresOriginalVariables]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvramfix.cpp:323]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvramFix.SetFailureStopsAndReports]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvramFix.SetFailureStopsAndReports]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvramFix.SetFailureStopsAndReports]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvramfix.cpp:343]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvramFix.MalformedDevicePathBounded]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvramFix.MalformedDevicePathBounded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvramFix.MalformedDevicePathBounded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvramfix.cpp:365]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvramFix.NullSeamRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvramFix.NullSeamRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvramFix.NullSeamRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvramfix.cpp:397]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvramFix.AbsentBootOrderIsSilentNoop]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvramFix.AbsentBootOrderIsSilentNoop]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvramFix.AbsentBootOrderIsSilentNoop]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvramfix.cpp:404]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvramFix.MissingBootVariableStopsTheWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvramFix.MissingBootVariableStopsTheWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvramFix.MissingBootVariableStopsTheWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvramfix.cpp:417]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvramFix.EmptyPlanIsNoOp]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=NvramFix.EmptyPlanIsNoOp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvramFix.EmptyPlanIsNoOp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvramfix.cpp:440]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Preview.ExtentAccurateReadsWithSparseTailAndEof]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Preview.ExtentAccurateReadsWithSparseTailAndEof]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Preview.ExtentAccurateReadsWithSparseTailAndEof]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:103]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Preview.BoundedReadCannotBeTricked]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Preview.BoundedReadCannotBeTricked]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Preview.BoundedReadCannotBeTricked]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:133]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Preview.ReadFailuresZeroFilledAndReported]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Preview.ReadFailuresZeroFilledAndReported]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Preview.ReadFailuresZeroFilledAndReported]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:155]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Preview.CompressedExtentDecodes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Preview.CompressedExtentDecodes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Preview.CompressedExtentDecodes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:172]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Preview.ConcurrentIndependentReaders]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Preview.ConcurrentIndependentReaders]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Preview.ConcurrentIndependentReaders]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:204]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Preview.MalformedCapsExtentKeepsAlignment]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Preview.MalformedCapsExtentKeepsAlignment]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Preview.MalformedCapsExtentKeepsAlignment]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1398]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Preview.M077SizeGate100MiB]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Preview.M077SizeGate100MiB]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Preview.M077SizeGate100MiB]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1454]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Preview.M077TimeGuardDefaultsOnAndOverridable]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Preview.M077TimeGuardDefaultsOnAndOverridable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Preview.M077TimeGuardDefaultsOnAndOverridable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1475]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.JpegDimensionsAndExif]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.JpegDimensionsAndExif]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.JpegDimensionsAndExif]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:234]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.PngGifBmpDimensions]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.PngGifBmpDimensions]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.PngGifBmpDimensions]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:296]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.WavDurationAndMp4Brand]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.WavDurationAndMp4Brand]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.WavDurationAndMp4Brand]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:352]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.AviMetadataExtracted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.AviMetadataExtracted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.AviMetadataExtracted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:409]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.MkvMetadataExtracted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.MkvMetadataExtracted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.MkvMetadataExtracted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:446]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.FlacMetadataExtracted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.FlacMetadataExtracted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.FlacMetadataExtracted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:492]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.Mp3DurationEstimated]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.Mp3DurationEstimated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.Mp3DurationEstimated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:523]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.M4aTypedAsMp4Family]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.M4aTypedAsMp4Family]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.M4aTypedAsMp4Family]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:553]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.PdfTextAndPlainAndUnsupported]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.PdfTextAndPlainAndUnsupported]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.PdfTextAndPlainAndUnsupported]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:574]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.TruncatedMagicsAreSafeAndHonest]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.TruncatedMagicsAreSafeAndHonest]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.TruncatedMagicsAreSafeAndHonest]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:652]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.WavFmtChunkOutsideHeadCache]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.WavFmtChunkOutsideHeadCache]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.WavFmtChunkOutsideHeadCache]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:702]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.DocxRawDeflateInflatesAndPlainZipStaysZip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.DocxRawDeflateInflatesAndPlainZipStaysZip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.DocxRawDeflateInflatesAndPlainZipStaysZip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:733]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.DocxCapsMaxReadHonored]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.DocxCapsMaxReadHonored]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.DocxCapsMaxReadHonored]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:964]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.DocxTailBeyondBudgetDegradesToZip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.DocxTailBeyondBudgetDegradesToZip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.DocxTailBeyondBudgetDegradesToZip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:989]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.CompressedExtentReportsTruncatedUnderBudget]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.CompressedExtentReportsTruncatedUnderBudget]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.CompressedExtentReportsTruncatedUnderBudget]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1008]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.CompressedExtentWholeBlockReadDisclosed]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.CompressedExtentWholeBlockReadDisclosed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.CompressedExtentWholeBlockReadDisclosed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1042]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.ArchiveEnumeratesEntries]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.ArchiveEnumeratesEntries]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.ArchiveEnumeratesEntries]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1168]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.ArchiveDamagedLocalHeaderStillEnumerates]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.ArchiveDamagedLocalHeaderStillEnumerates]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.ArchiveDamagedLocalHeaderStillEnumerates]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1195]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.ArchiveDocxUpgradeCoexistsWithEnumeration]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.ArchiveDocxUpgradeCoexistsWithEnumeration]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.ArchiveDocxUpgradeCoexistsWithEnumeration]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1224]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.ArchiveEntryCapAndTruncatedFlag]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.ArchiveEntryCapAndTruncatedFlag]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.ArchiveEntryCapAndTruncatedFlag]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1312]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.MetadataTruncatedFlagTruthfulAllPaths]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.MetadataTruncatedFlagTruthfulAllPaths]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.MetadataTruncatedFlagTruthfulAllPaths]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1377]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.RandomSmallInputsAreSafe]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.RandomSmallInputsAreSafe]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.RandomSmallInputsAreSafe]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1421]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.Utf8BomTextIsText]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.Utf8BomTextIsText]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.Utf8BomTextIsText]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1492]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.Utf16BomTextIsText]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.Utf16BomTextIsText]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.Utf16BomTextIsText]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1513]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.Utf16SurrogatePairDecodes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.Utf16SurrogatePairDecodes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.Utf16SurrogatePairDecodes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1534]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.Latin1TextIsText]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.Latin1TextIsText]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.Latin1TextIsText]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1561]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.PrintableHeadBinaryNotText]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.PrintableHeadBinaryNotText]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.PrintableHeadBinaryNotText]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1576]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.ExtensionlessNameIsNotText]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.ExtensionlessNameIsNotText]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.ExtensionlessNameIsNotText]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1594]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.WhitelistMembershipCaseInsensitive]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewMeta.WhitelistMembershipCaseInsensitive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.WhitelistMembershipCaseInsensitive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1608]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewDocs.NoIInWindowStreamDriftAndNoMovGhost]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewDocs.NoIInWindowStreamDriftAndNoMovGhost]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewDocs.NoIInWindowStreamDriftAndNoMovGhost]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1334]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewTextWhitelist.ByteExactAgainstCorpus]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=PreviewTextWhitelist.ByteExactAgainstCorpus]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewTextWhitelist.ByteExactAgainstCorpus]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:1642]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.StatusNamesAreStable]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.StatusNamesAreStable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.StatusNamesAreStable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:111]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.TextSnapshotRenders]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.TextSnapshotRenders]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.TextSnapshotRenders]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:144]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.TextSnapshotHundredByteBound]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.TextSnapshotHundredByteBound]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.TextSnapshotHundredByteBound]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:175]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.TextSnapshotControlsAreBlankAndEmptyRefuses]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.TextSnapshotControlsAreBlankAndEmptyRefuses]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.TextSnapshotControlsAreBlankAndEmptyRefuses]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:189]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.TextSnapshotUtf16BomDecodes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.TextSnapshotUtf16BomDecodes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.TextSnapshotUtf16BomDecodes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:212]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.FormatSupportIsTheDocumentedV1Matrix]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.FormatSupportIsTheDocumentedV1Matrix]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.FormatSupportIsTheDocumentedV1Matrix]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:232]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.MetafileAndIconFamiliesAreNamedRefusals]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.MetafileAndIconFamiliesAreNamedRefusals]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.MetafileAndIconFamiliesAreNamedRefusals]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:247]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.CapsDefaultsPinTheCorpusAnchors]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.CapsDefaultsPinTheCorpusAnchors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.CapsDefaultsPinTheCorpusAnchors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:294]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.PngDecodesAndDownscalesExactly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.PngDecodesAndDownscalesExactly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.PngDecodesAndDownscalesExactly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:305]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.OrdinaryPhotoDefaultBudget]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.OrdinaryPhotoDefaultBudget]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.OrdinaryPhotoDefaultBudget]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:447]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.EmbeddedFastPathOnByteOversize]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.EmbeddedFastPathOnByteOversize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.EmbeddedFastPathOnByteOversize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:472]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.RawTiffFamilyServesJpegPreview]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.RawTiffFamilyServesJpegPreview]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.RawTiffFamilyServesJpegPreview]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:571]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.JpegDecodesWithLossyTolerance]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.JpegDecodesWithLossyTolerance]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.JpegDecodesWithLossyTolerance]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:593]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.BmpDecodesExactly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.BmpDecodesExactly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.BmpDecodesExactly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:612]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.GifDecodesFirstFrameExactly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.GifDecodesFirstFrameExactly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.GifDecodesFirstFrameExactly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:627]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.AspectPreservedBothAxesNeverUpscaled]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.AspectPreservedBothAxesNeverUpscaled]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.AspectPreservedBothAxesNeverUpscaled]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:681]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.SmallImageIsNotUpscaled]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.SmallImageIsNotUpscaled]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.SmallImageIsNotUpscaled]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:699]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.BombGuardRefusesBeforeDecode]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.BombGuardRefusesBeforeDecode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.BombGuardRefusesBeforeDecode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:714]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.OversizedJpegUsesExifEmbeddedThumbnail]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.OversizedJpegUsesExifEmbeddedThumbnail]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.OversizedJpegUsesExifEmbeddedThumbnail]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:728]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.SmallJpegWithEmbeddedThumbPrefersFullDecode]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.SmallJpegWithEmbeddedThumbPrefersFullDecode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.SmallJpegWithEmbeddedThumbPrefersFullDecode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:745]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.TiffOrientationApplied]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.TiffOrientationApplied]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.TiffOrientationApplied]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:763]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.TiffOrientationInvalidIgnored]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.TiffOrientationInvalidIgnored]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.TiffOrientationInvalidIgnored]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:781]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.EmbeddedFastPathServesStoredOrientationByDefault]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.EmbeddedFastPathServesStoredOrientationByDefault]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.EmbeddedFastPathServesStoredOrientationByDefault]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:793]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.EmbeddedFastPathHonorsIfd1DeclaredOrientation]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.EmbeddedFastPathHonorsIfd1DeclaredOrientation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.EmbeddedFastPathHonorsIfd1DeclaredOrientation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:809]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.TruncatedPngFailsHonestly]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.TruncatedPngFailsHonestly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.TruncatedPngFailsHonestly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:829]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.TruncatedJpegIsDisclosedNotSilent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.TruncatedJpegIsDisclosedNotSilent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.TruncatedJpegIsDisclosedNotSilent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:841]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.BudgetExceededIsReported]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.BudgetExceededIsReported]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.BudgetExceededIsReported]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:858]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.UnsupportedFormatsAreNamed]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.UnsupportedFormatsAreNamed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.UnsupportedFormatsAreNamed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:871]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.UnreadableSpanIsNeverThumbnailed]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.UnreadableSpanIsNeverThumbnailed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.UnreadableSpanIsNeverThumbnailed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:894]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.BadParametersAreRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.BadParametersAreRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.BadParametersAreRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:908]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.EmptyInputIsUnsupported]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.EmptyInputIsUnsupported]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.EmptyInputIsUnsupported]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:925]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.ExtentBackedDecodesLikeResident]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.ExtentBackedDecodesLikeResident]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.ExtentBackedDecodesLikeResident]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:934]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.LargeStillImageWithinOgBand]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.LargeStillImageWithinOgBand]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.LargeStillImageWithinOgBand]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:1029]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Thumbnail.OversizePastOgGateRefused]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Thumbnail.OversizePastOgGateRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Thumbnail.OversizePastOgGateRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:1050]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Snapshot.CaptureFidelityAndDfsParity]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Snapshot.CaptureFidelityAndDfsParity]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Snapshot.CaptureFidelityAndDfsParity]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_snapshot.cpp:112]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Snapshot.PartialFlagAndEmptyTree]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Snapshot.PartialFlagAndEmptyTree]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Snapshot.PartialFlagAndEmptyTree]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_snapshot.cpp:182]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Snapshot.ConcurrentEmitCaptureWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Snapshot.ConcurrentEmitCaptureWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Snapshot.ConcurrentEmitCaptureWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_snapshot.cpp:193]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Snapshot.RecoveryFromSnapshotIsByteIdentical]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Snapshot.RecoveryFromSnapshotIsByteIdentical]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Snapshot.RecoveryFromSnapshotIsByteIdentical]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_snapshot.cpp:243]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.CategoryMap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.CategoryMap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.CategoryMap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:85]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.EmailCategoryExists]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.EmailCategoryExists]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.EmailCategoryExists]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:97]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.BookmarksCategory]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.BookmarksCategory]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.BookmarksCategory]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:107]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.CameraRawClassified]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.CameraRawClassified]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.CameraRawClassified]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:111]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.TableCountMatchesCorpusUniques]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.TableCountMatchesCorpusUniques]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.TableCountMatchesCorpusUniques]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:125]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.CrossListedFirstWriterWins]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.CrossListedFirstWriterWins]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.CrossListedFirstWriterWins]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:131]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.FilterSortPageCorrectness]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.FilterSortPageCorrectness]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.FilterSortPageCorrectness]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:143]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.CarvedVolumeFilterAxis]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.CarvedVolumeFilterAxis]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.CarvedVolumeFilterAxis]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:265]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.PathPrefixFilterMatchesAndPrunes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.PathPrefixFilterMatchesAndPrunes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.PathPrefixFilterMatchesAndPrunes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:316]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.M072PathSubstringAndStateMask]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.M072PathSubstringAndStateMask]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.M072PathSubstringAndStateMask]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:397]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.RandomizedAgainstReference]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.RandomizedAgainstReference]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.RandomizedAgainstReference]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:474]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.TypeSortKeyOrdersByCategoryThenExtension]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.TypeSortKeyOrdersByCategoryThenExtension]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.TypeSortKeyOrdersByCategoryThenExtension]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:507]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.ModifiedUnitIsFiletimeEverywhere]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.ModifiedUnitIsFiletimeEverywhere]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.ModifiedUnitIsFiletimeEverywhere]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:553]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.DateBucketsTranslateToRanges]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.DateBucketsTranslateToRanges]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.DateBucketsTranslateToRanges]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:674]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.SizeBucketsTranslateToRanges]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.SizeBucketsTranslateToRanges]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.SizeBucketsTranslateToRanges]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:739]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.BucketFiltersThroughRun]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.BucketFiltersThroughRun]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.BucketFiltersThroughRun]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:799]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.UnsavedFileCategory]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.UnsavedFileCategory]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.UnsavedFileCategory]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:884]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.CategoriesAppendOnlyGuard]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Query.CategoriesAppendOnlyGuard]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.CategoriesAppendOnlyGuard]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:915]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Error.ErrTextCoversEveryCode]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Error.ErrTextCoversEveryCode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Error.ErrTextCoversEveryCode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:585]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Display.SizeLogFormatterBoundaries]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Display.SizeLogFormatterBoundaries]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Display.SizeLogFormatterBoundaries]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:933]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Display.SizeUiFormatterTiers]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Display.SizeUiFormatterTiers]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Display.SizeUiFormatterTiers]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:954]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Display.SizeUiFormatterPrecisionAndStrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Display.SizeUiFormatterPrecisionAndStrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Display.SizeUiFormatterPrecisionAndStrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:971]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.FatCopyDisagreementReported]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.FatCopyDisagreementReported]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.FatCopyDisagreementReported]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:132]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.FatDuplicateRefDetected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.FatDuplicateRefDetected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.FatDuplicateRefDetected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:148]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.FatEntryHoleReported]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.FatEntryHoleReported]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.FatEntryHoleReported]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:165]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.FatCrossLinkedChainReported]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.FatCrossLinkedChainReported]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.FatCrossLinkedChainReported]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:183]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.DirDotPairMissingRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.DirDotPairMissingRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.DirDotPairMissingRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:202]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.DirCycleDetected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.DirCycleDetected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.DirCycleDetected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:219]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.LiveFileSizeNeChain]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.LiveFileSizeNeChain]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.LiveFileSizeNeChain]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:249]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.OrphanClustersReported]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.OrphanClustersReported]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.OrphanClustersReported]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:262]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.CleanFatVolumeReportsNoErrors]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.CleanFatVolumeReportsNoErrors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.CleanFatVolumeReportsNoErrors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:278]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.DbrCeilingRejectsOversizedClusterCount]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.DbrCeilingRejectsOversizedClusterCount]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.DbrCeilingRejectsOversizedClusterCount]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:291]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.NtfsCleanVolumeReportsNoErrors]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.NtfsCleanVolumeReportsNoErrors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.NtfsCleanVolumeReportsNoErrors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:504]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.NtfsVersionGate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.NtfsVersionGate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.NtfsVersionGate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:524]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.MftRecordNotNamedMft]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.MftRecordNotNamedMft]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.MftRecordNotNamedMft]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:542]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.MftBitmapCrossLink]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.MftBitmapCrossLink]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.MftBitmapCrossLink]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:565]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.BitmapFreeButAllocated]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.BitmapFreeButAllocated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.BitmapFreeButAllocated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:591]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.CheckNeverAbortsScan]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.CheckNeverAbortsScan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.CheckNeverAbortsScan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:617]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FsCheck.ReportSessionRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=FsCheck.ReportSessionRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FsCheck.ReportSessionRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fscheck.cpp:638]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[QueryPerf.MillionNodesFilterCountUnder200ms]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=QueryPerf.MillionNodesFilterCountUnder200ms]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[QueryPerf.MillionNodesFilterCountUnder200ms]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query_perf.cpp:34]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DiskMap.RangeFreeEdgeMatrix]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DiskMap.RangeFreeEdgeMatrix]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DiskMap.RangeFreeEdgeMatrix]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_diskmap.cpp:41]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DiskMap.FreeMapRecordRoundTripAndRejection]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DiskMap.FreeMapRecordRoundTripAndRejection]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DiskMap.FreeMapRecordRoundTripAndRejection]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_diskmap.cpp:73]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DiskMap.FreeOverlayHonorsZoomWindow]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DiskMap.FreeOverlayHonorsZoomWindow]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DiskMap.FreeOverlayHonorsZoomWindow]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_diskmap.cpp:111]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DiskMap.DownsamplePreservesRawTruth]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DiskMap.DownsamplePreservesRawTruth]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DiskMap.DownsamplePreservesRawTruth]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_diskmap.cpp:138]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DiskMap.FreeNeverMarksFoundContentCells]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DiskMap.FreeNeverMarksFoundContentCells]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DiskMap.FreeNeverMarksFoundContentCells]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_diskmap.cpp:165]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DiskMap.OverlayBudgetGrowsCellSize]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DiskMap.OverlayBudgetGrowsCellSize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DiskMap.OverlayBudgetGrowsCellSize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_diskmap.cpp:205]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DiskMap.FreeNeverOverridesUsedOrBadAndDownsampleIsConservative]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=DiskMap.FreeNeverOverridesUsedOrBadAndDownsampleIsConservative]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DiskMap.FreeNeverOverridesUsedOrBadAndDownsampleIsConservative]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_diskmap.cpp:242]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.PlansEntryIntoFirstFreeSlot]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Apply.PlansEntryIntoFirstFreeSlot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.PlansEntryIntoFirstFreeSlot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:167]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.ExecuteWritesEntryVerifiesAndUndoes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Apply.ExecuteWritesEntryVerifiesAndUndoes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.ExecuteWritesEntryVerifiesAndUndoes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:187]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.RefusedWithoutConsent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Apply.RefusedWithoutConsent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.RefusedWithoutConsent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:219]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.StaleTableRefusedBeforeConsent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Apply.StaleTableRefusedBeforeConsent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.StaleTableRefusedBeforeConsent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:239]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.QuiesceFailureIsACleanRefusal]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Apply.QuiesceFailureIsACleanRefusal]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.QuiesceFailureIsACleanRefusal]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:262]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.WriteFailureRollsBackEverything]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Apply.WriteFailureRollsBackEverything]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.WriteFailureRollsBackEverything]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:277]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.ContainmentRefusals]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Apply.ContainmentRefusals]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.ContainmentRefusals]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:302]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.SignatureRestorePlannedWhenAbsent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Apply.SignatureRestorePlannedWhenAbsent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.SignatureRestorePlannedWhenAbsent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:363]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.ChsClampsBeyondCylinder1023]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Apply.ChsClampsBeyondCylinder1023]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.ChsClampsBeyondCylinder1023]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:388]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.FamilyTypeMap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Apply.FamilyTypeMap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.FamilyTypeMap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:414]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.WritesEntryAndRefreshesModernPath]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostApply.WritesEntryAndRefreshesModernPath]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.WritesEntryAndRefreshesModernPath]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:580]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.LegacyPathMatchesCorpusSequence]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostApply.LegacyPathMatchesCorpusSequence]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.LegacyPathMatchesCorpusSequence]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:622]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.GptRebuildRegeneratesSignature]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostApply.GptRebuildRegeneratesSignature]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.GptRebuildRegeneratesSignature]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:656]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.ConsentBeforeLockDenialHoldsNothing]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostApply.ConsentBeforeLockDenialHoldsNothing]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.ConsentBeforeLockDenialHoldsNothing]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:696]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.UndoApplyRestoresAndRefreshes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostApply.UndoApplyRestoresAndRefreshes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.UndoApplyRestoresAndRefreshes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:719]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.RefreshFailureIsReportedNotSwallowed]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostApply.RefreshFailureIsReportedNotSwallowed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.RefreshFailureIsReportedNotSwallowed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:742]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.RollsBackWhenVerifyFails]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostApply.RollsBackWhenVerifyFails]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.RollsBackWhenVerifyFails]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:765]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.PlanIsReadOnlyAndRefusalsAreResults]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=LostApply.PlanIsReadOnlyAndRefusalsAreResults]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.PlanIsReadOnlyAndRefusalsAreResults]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:783]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.CleanExfatNeverTouched]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.CleanExfatNeverTouched]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.CleanExfatNeverTouched]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:154]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExfatChecksumRepair]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.ExfatChecksumRepair]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExfatChecksumRepair]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:177]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExfatDestroyedPrimaryRestoredFromBackup]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.ExfatDestroyedPrimaryRestoredFromBackup]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExfatDestroyedPrimaryRestoredFromBackup]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:212]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.NtfsBackupRestore]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.NtfsBackupRestore]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.NtfsBackupRestore]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:250]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.NtfsCleanRefused]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.NtfsCleanRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.NtfsCleanRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:286]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.NtfsNoBackupRefused]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.NtfsNoBackupRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.NtfsNoBackupRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:306]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.Fat32BackupRestore]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.Fat32BackupRestore]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.Fat32BackupRestore]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:319]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExfatGeometryRederivation]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.ExfatGeometryRederivation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExfatGeometryRederivation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:357]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExfatAmbiguousGeometryRefused]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.ExfatAmbiguousGeometryRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExfatAmbiguousGeometryRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:383]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.UnknownFamilyRefused]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.UnknownFamilyRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.UnknownFamilyRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:406]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExecuteRefusesEmptyPlan]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.ExecuteRefusesEmptyPlan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExecuteRefusesEmptyPlan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:416]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FailedWriteRollsBackEverything]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.FailedWriteRollsBackEverything]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FailedWriteRollsBackEverything]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:424]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.UndoLogRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.UndoLogRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.UndoLogRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:458]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.MftRecord0RestoredFromMirror]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.MftRecord0RestoredFromMirror]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.MftRecord0RestoredFromMirror]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:609]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.MftCleanNeverTouched]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.MftCleanNeverTouched]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.MftCleanNeverTouched]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:634]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.MftBothInvalidNoWrite]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.MftBothInvalidNoWrite]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.MftBothInvalidNoWrite]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:644]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.MftRestoreMakesVolumeOpenable]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.MftRestoreMakesVolumeOpenable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.MftRestoreMakesVolumeOpenable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:659]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FatTornMirrorCopyRestoredSectorGranular]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.FatTornMirrorCopyRestoredSectorGranular]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FatTornMirrorCopyRestoredSectorGranular]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:698]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FatTornPrimaryRestoredFromMirror]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.FatTornPrimaryRestoredFromMirror]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FatTornPrimaryRestoredFromMirror]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:731]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FatMirrorAmbiguityNeverWritten]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.FatMirrorAmbiguityNeverWritten]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FatMirrorAmbiguityNeverWritten]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:748]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FatCleanMirrorNoAction]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.FatCleanMirrorNoAction]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FatCleanMirrorNoAction]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:765]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FixByScanSectorRebuildsBootFromOnlyCandidate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.FixByScanSectorRebuildsBootFromOnlyCandidate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FixByScanSectorRebuildsBootFromOnlyCandidate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:774]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FixByScanSectorAmbiguousCandidatesRefused]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.FixByScanSectorAmbiguousCandidatesRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FixByScanSectorAmbiguousCandidatesRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:801]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FixByScanSectorIdenticalCandidatesNotAmbiguous]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.FixByScanSectorIdenticalCandidatesNotAmbiguous]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FixByScanSectorIdenticalCandidatesNotAmbiguous]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:816]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.RebasedPartitionPlanWritesAtDiskBasisNeverLba0]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.RebasedPartitionPlanWritesAtDiskBasisNeverLba0]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.RebasedPartitionPlanWritesAtDiskBasisNeverLba0]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:835]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.VolumeLockAcquiredBeforeFirstWrite]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.VolumeLockAcquiredBeforeFirstWrite]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.VolumeLockAcquiredBeforeFirstWrite]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:889]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.VolumeLockFailureNeverWrites]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.VolumeLockFailureNeverWrites]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.VolumeLockFailureNeverWrites]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:908]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.UnlockHappensOnRollbackPath]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.UnlockHappensOnRollbackPath]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.UnlockHappensOnRollbackPath]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:928]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.NtfsPrimaryDamaged4Kn]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.NtfsPrimaryDamaged4Kn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.NtfsPrimaryDamaged4Kn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:1000]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExfatViaBackup4Kn]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.ExfatViaBackup4Kn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExfatViaBackup4Kn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:1032]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.AnalyzeAllBranches4KnAsan]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.AnalyzeAllBranches4KnAsan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.AnalyzeAllBranches4KnAsan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:1065]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExfatRestorePlanIsSectorSized4Kn]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.ExfatRestorePlanIsSectorSized4Kn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExfatRestorePlanIsSectorSized4Kn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:1116]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExfatBackupReadFull4Kn]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=RawFixer.ExfatBackupReadFull4Kn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExfatBackupReadFull4Kn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:1151]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Consistency.ExtensionIsLastDotUppercased]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Consistency.ExtensionIsLastDotUppercased]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Consistency.ExtensionIsLastDotUppercased]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:61]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Consistency.CheckableVocabularyIsSignatureFamiliesPlusSpecials]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Consistency.CheckableVocabularyIsSignatureFamiliesPlusSpecials]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Consistency.CheckableVocabularyIsSignatureFamiliesPlusSpecials]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:69]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Consistency.ClassifierIsSubstringPlusVideoSpecials]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Consistency.ClassifierIsSubstringPlusVideoSpecials]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Consistency.ClassifierIsSubstringPlusVideoSpecials]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:92]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.ContentInconsistentGhostRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Lost.ContentInconsistentGhostRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.ContentInconsistentGhostRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:134]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.ContentConsistentGhostAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Lost.ContentConsistentGhostAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.ContentConsistentGhostAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:157]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.RatioBoundaryThirtyPercent]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Lost.RatioBoundaryThirtyPercent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.RatioBoundaryThirtyPercent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:174]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.MajorityGateRequiresHalfFilterable]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Lost.MajorityGateRequiresHalfFilterable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.MajorityGateRequiresHalfFilterable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:199]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.DirectoriesAndExtentlessFilesMirrorOgCounters]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Lost.DirectoriesAndExtentlessFilesMirrorOgCounters]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.DirectoriesAndExtentlessFilesMirrorOgCounters]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:218]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.DataStartOutOfBoundsInvalidatesTheCandidate]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Lost.DataStartOutOfBoundsInvalidatesTheCandidate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.DataStartOutOfBoundsInvalidatesTheCandidate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:245]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.UnreadableDataStartStaysUnmatched]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Lost.UnreadableDataStartStaysUnmatched]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.UnreadableDataStartStaysUnmatched]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:256]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.EmptyCandidateListNeverRejects]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Lost.EmptyCandidateListNeverRejects]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.EmptyCandidateListNeverRejects]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:279]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.WholeDiskScanDropsContentInconsistentExfatGhost]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Lost.WholeDiskScanDropsContentInconsistentExfatGhost]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.WholeDiskScanDropsContentInconsistentExfatGhost]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:538]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.WholeDiskScanKeepsContentConsistentExfatGhost]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Lost.WholeDiskScanKeepsContentConsistentExfatGhost]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.WholeDiskScanKeepsContentConsistentExfatGhost]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:587]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[TreeDetach.RemovesNodesRootsAndIndexes]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=TreeDetach.RemovesNodesRootsAndIndexes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TreeDetach.RemovesNodesRootsAndIndexes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:304]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[TreeDetach.HardLinkPropsSurviveWithTheirRefs]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=TreeDetach.HardLinkPropsSurviveWithTheirRefs]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TreeDetach.HardLinkPropsSurviveWithTheirRefs]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:343]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.BootValidation]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.BootValidation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.BootValidation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:508]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.FullScanHierarchy]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.FullScanHierarchy]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.FullScanHierarchy]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:523]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.FallbackRootSkipsOgSystemFamilies]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.FallbackRootSkipsOgSystemFamilies]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.FallbackRootSkipsOgSystemFamilies]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:574]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.DeletedGradingAndId2]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.DeletedGradingAndId2]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.DeletedGradingAndId2]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:630]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.OrchestratorProbeAndScan]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.OrchestratorProbeAndScan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.OrchestratorProbeAndScan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:666]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.RecognitionRequiresFsrsMarker]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.RecognitionRequiresFsrsMarker]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.RecognitionRequiresFsrsMarker]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:681]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.NextChainFindsNewerTable]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.NextChainFindsNewerTable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.NextChainFindsNewerTable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:702]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.CheckpointLayoutMinor14]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.CheckpointLayoutMinor14]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.CheckpointLayoutMinor14]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:790]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.Pre34CheckpointRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.Pre34CheckpointRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.Pre34CheckpointRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:831]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.CheckpointBodyMinorGatesLayoutNotVbr]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.CheckpointBodyMinorGatesLayoutNotVbr]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.CheckpointBodyMinorGatesLayoutNotVbr]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:846]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.VbrMinor14BodyMinorOldListStillOldLayout]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.VbrMinor14BodyMinorOldListStillOldLayout]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.VbrMinor14BodyMinorOldListStillOldLayout]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:888]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.ResidentInlineFileRecovered]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.ResidentInlineFileRecovered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.ResidentInlineFileRecovered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:978]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.ResidentDollarIMetadataRead]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.ResidentDollarIMetadataRead]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.ResidentDollarIMetadataRead]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1027]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.MultipleDataAttributesMerged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.MultipleDataAttributesMerged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.MultipleDataAttributesMerged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1069]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.PropertyFlagAndUnknownRowsDoNotBreakDataWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.PropertyFlagAndUnknownRowsDoNotBreakDataWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.PropertyFlagAndUnknownRowsDoNotBreakDataWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1103]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.PagedRunTableFollowed]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.PagedRunTableFollowed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.PagedRunTableFollowed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1173]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.Gen7OldGrammarDatarunAttribute]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.Gen7OldGrammarDatarunAttribute]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.Gen7OldGrammarDatarunAttribute]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1221]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.RunlistGranularityMismatchDiscarded]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.RunlistGranularityMismatchDiscarded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.RunlistGranularityMismatchDiscarded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1255]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.StaleMsbPageDeletedFileRecovered]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.StaleMsbPageDeletedFileRecovered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.StaleMsbPageDeletedFileRecovered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1287]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.StaleSweepSkipsLivePages]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.StaleSweepSkipsLivePages]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.StaleSweepSkipsLivePages]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1324]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.HardLinkRowsCollapseToOneResult]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.HardLinkRowsCollapseToOneResult]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.HardLinkRowsCollapseToOneResult]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1350]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.RecycleRoutedFlaggedAndDollarIRemap]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.RecycleRoutedFlaggedAndDollarIRemap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.RecycleRoutedFlaggedAndDollarIRemap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1381]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.ProgressMonotonicIntermediateTicks]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.ProgressMonotonicIntermediateTicks]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.ProgressMonotonicIntermediateTicks]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1478]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.ProgressCancellationMidWalk]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.ProgressCancellationMidWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.ProgressCancellationMidWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1518]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.Bps256Accepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.Bps256Accepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.Bps256Accepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1563]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.Bps1024Accepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.Bps1024Accepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.Bps1024Accepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1577]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.Cluster16KRejected]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.Cluster16KRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.Cluster16KRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1589]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.Cluster64KAccepted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.Cluster64KAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.Cluster64KAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1600]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.OpenReadIsFourKiB]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.OpenReadIsFourKiB]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.OpenReadIsFourKiB]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1612]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.ContainerRelativeRunTranslatedPerOgFormula]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.ContainerRelativeRunTranslatedPerOgFormula]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.ContainerRelativeRunTranslatedPerOgFormula]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1659]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.PhysicalRunFlag20Untranslated]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.PhysicalRunFlag20Untranslated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.PhysicalRunFlag20Untranslated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1688]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.ContainerBlockBelowSpanStaysPhysical]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.ContainerBlockBelowSpanStaysPhysical]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.ContainerBlockBelowSpanStaysPhysical]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1714]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.SingleContainerExtentsUnchanged]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.SingleContainerExtentsUnchanged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.SingleContainerExtentsUnchanged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1740]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.UnnamedDataOnlyRowEmitted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.UnnamedDataOnlyRowEmitted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.UnnamedDataOnlyRowEmitted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1859]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.UnnamedRowGradeIsContentBased]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.UnnamedRowGradeIsContentBased]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.UnnamedRowGradeIsContentBased]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1899]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.UnnamedOddLengthKeyEmittedNoCrash]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.UnnamedOddLengthKeyEmittedNoCrash]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.UnnamedOddLengthKeyEmittedNoCrash]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1930]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.ZeroSizeNoDotRowSuppressed]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.ZeroSizeNoDotRowSuppressed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.ZeroSizeNoDotRowSuppressed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1956]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.ZeroSizeWithExtensionEmitted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.ZeroSizeWithExtensionEmitted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.ZeroSizeWithExtensionEmitted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1994]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.NonZeroNoDotEmitted]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.NonZeroNoDotEmitted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.NonZeroNoDotEmitted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:2024]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.UnnamedZeroSizeRowSuppressed]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.UnnamedZeroSizeRowSuppressed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.UnnamedZeroSizeRowSuppressed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:2053]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.UnnamedNameSessionRoundTrip]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=Refs.UnnamedNameSessionRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.UnnamedNameSessionRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:2081]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/1]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/0]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/1]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:340]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/2]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/1]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/2]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:340]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/3]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/2]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/3]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:340]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/4]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/3]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/4]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:340]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/5]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/4]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/5]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:340]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/6]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/5]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/6]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:340]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/7]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/6]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/7]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:340]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/8]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/7]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/8]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:340]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/9]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/8]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/9]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:340]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_rgb_le.tif"]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/0]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_rgb_le.tif"]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:644]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_rgb_be.tif"]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/1]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_rgb_be.tif"]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:644]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_packbits.tif"]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/2]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_packbits.tif"]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:644]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_deflate.tif"]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/3]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_deflate.tif"]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:644]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_palette.tif"]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/4]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_palette.tif"]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:644]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_gray16.tif"]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/5]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_gray16.tif"]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:644]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_lzw.tif"]=]  /home/z/my-project/work/core_recovery/build2/tests/cr_tests [==[--gtest_filter=TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/6]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_lzw.tif"]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_thumbnail.cpp:644]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build2/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
set(cr_tests_TESTS [==[Fixup.ValidRecordApplies]==] [==[Fixup.TornRecordRejected]==] [==[Fixup.TornThirdTailStillSalvaged]==] [==[Fixup.MinSizeGate]==] [==[Fixup.ZeroOffsetRejected]==] [==[Fixup.FourKnSectorSize]==] [==[Image.FlatFileScansMbr]==] [==[Image.FlatRejectsEmptyAndMissing]==] [==[Image.TruncatedFlatImageCeilsSectorCount]==] [==[Image.FlatGeometryValidOnOddSizedImage]==] [==[Image.ImageRejectsTruncatedFile]==] [==[Image.EdiskRefusedAsRegisteredExclusion]==] [==[Image.BadMagicVmdkRefusedNotFlatFallback]==] [==[Image.VmdkSparseExtentReads]==] [==[Image.VmdkDifferencingChainReadsParent]==] [==[Image.VmdkStreamOptimizedRefused]==] [==[Image.VhdDynamicDisk]==] [==[Image.VhdDifferencingReadsParentOnClearBitmap]==] [==[Image.VhdBadChecksumRefused]==] [==[Image.VhdxDynamicReads]==] [==[Image.VhdxLogOverlayServesUnflushedWrites]==] [==[Image.VhdxDifferencingRefusedPrecisely]==] [==[Image.Lba0LayoutClassification]==] [==[Image.ExtensionClassificationMatchesOgDispatch]==] [==[Image.SessionRoundTripPersistsImagePath]==] [==[Image.QemuVmdkCompat]==] [==[Image.QemuVhdCompat]==] [==[Image.QemuVhdxCompat]==] [==[ImageCreate.HashMatchesSource]==] [==[ImageCreate.ShortTailNotPadded]==] [==[ImageCreate.FourKnGeometryStreamsUnpadded]==] [==[ImageCreate.CancelMidStreamKeepsTruthfulPartialStats]==] [==[ImageCreate.ReadFailureReportsCommittedPrefix]==] [==[ImageCreate.ProgressCallbackPerChunk]==] [==[ImageCreate.EmptyDeviceIsEmptyStream]==] [==[NvmeHealth.PageFieldOffsets]==] [==[NvmeHealth.FullFieldSurface]==] [==[NvmeHealth.DataUnitsConvertedToBytes]==] [==[NvmeHealth.ShortBufferRejected]==] [==[NvmeHealth.NullRejected]==] [==[NvmeHealth.PercentageUsedIsU8Width]==] [==[Nvme.CliTempFractionIsCelsiusDecidegrees]==] [==[Health.SmartCommandListMatchesImplementation]==] [==[Health.Attr197Raw3IsCaution]==] [==[Health.ValueBelowVendorThresholdIsBad]==] [==[Health.Attr5RawAllFFSkipsCautionCheck]==] [==[Health.DuplicateAttrIdYieldsUnknown]==] [==[Health.NoGradedSignalYieldsUnknown]==] [==[Health.SsdAnyAttrBelowThresholdIsBad]==] [==[Health.TemperatureNeverAffectsTheDiskGrade]==] [==[Health.SsdLifeFamilyGatesAndBoundaries]==] [==[Health.SsdLifeBelowVendorThresholdIsBad]==] [==[Health.SsdLifeInvertFormReads100MinusRaw]==] [==[Health.WdcAttr230LifeFromRawByte]==] [==[Health.Attr1RuleIsSandForceGated]==] [==[Health.FailingBitForcesDiskBadButAttrGradesFollowOg]==] [==[Health.NvmeGradeMapping]==] [==[Health.ModelSaysSsdOcList]==] [==[Health.ClassifyFamilyDecodedVendors]==] [==[Health.ClassifyFamilyAttributeSignatures]==] [==[Health.ClassifyFamilyFormsAndFallbacks]==] [==[Health.MakeGradeInputsAppliesOgDefaults]==] [==[Health.GradeNames]==] [==[Health.CsmiRequestMatchesOgWireFormat]==] [==[Health.CsmiIdentifyCarriesZeroLbaAndSmartCarriesMagic]==] [==[Health.CsmiBuilderRejectsUndersizedBuffer]==] [==[Health.CsmiResponseDataExtractedAtOgOffset]==] [==[Health.CsmiTransportUnavailableClassifiesTheOgErrorSet]==] [==[Health.CsmiTransportFallsBack]==] [==[Health.CsmiReadsAttributes]==] [==[Health.SmartChainEmptyIsBadParameter]==] [==[Health.SmartAttrNameTableCoversTheCuratedIds]==] [==[RunList.SingleRunDecode]==] [==[RunList.NegativeDelta]==] [==[RunList.SparseHoleEmitsExtent]==] [==[RunList.ContiguousCoalesceRequiresLogicalContiguityToo]==] [==[RunList.SparseRun]==] [==[RunList.TerminatesAndRejectsTruncated]==] [==[RunList.TornAfterValidRunsKeepsPrefix]==] [==[RunList.AllGarbageYieldsNothing]==] [==[RunList.SparseFirstAcceptedAndRegistered]==] [==[RunList.ZeroLengthRunRejected]==] [==[RunList.MaxClusterGuard]==] [==[RunList.EncodeDecodeRoundTrip]==] [==[RunList.SignedWidthEdgeCases]==] [==[RunList.ExtentCoalesce]==] [==[RunList.Int64MinOffsetRejected]==] [==[Crc32.KnownVectors]==] [==[Gpt.ValidTable]==] [==[Gpt.BadHeaderCrcDetected]==] [==[Gpt.DeadPrimaryHeaderSalvagesOwnEntries]==] [==[Gpt.ProtectiveOnlyVolumeRecoversFromBackup]==] [==[Gpt.EntryLastLbaBeforeFirstLbaSkippedAndCounted]==] [==[Gpt.EntrySpanOverflowBoundary]==] [==[Gpt.HostileSalvageHeaderEntriesOutsideDiskRejected]==] [==[Gpt.CanonicalTypeGuidConstantsMatchRealDiskBytes]==] [==[Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText]==] [==[Gpt.EspMsrLinuxBiosbootLdmClassification]==] [==[Gpt.OgGateSetHealthyHeaderTrusted]==] [==[Gpt.HeaderSizeNot92Rejected]==] [==[Gpt.ReservedNonzeroRejected]==] [==[Gpt.WrongRevisionRejected]==] [==[Gpt.PrimaryCurrentLbaWrongRejected]==] [==[Gpt.BackupLbaWrongRejected]==] [==[Gpt.UsableRangeBeyondDeviceRejected]==] [==[Gpt.EntriesLbaInsideUsableRangeRejected]==] [==[Gpt.TruncatedImageFallsToLadderNotTrustedPrimary]==] [==[Gpt.BackupProbedAtDeviceLastWhenPrimaryOk]==] [==[Gpt.CorruptBackupLbaFieldStillFindsBackup]==] [==[Gpt.DeadLba1BackupRecoveryRegistered]==] [==[Mbr.PlainWithExtended]==] [==[Mbr.LinuxExtended085LinksFollowTheChain]==] [==[Mbr.EbrLayoutAssignedAndPlainMbrStaysMbr]==] [==[Mbr.EbrCycleGuard]==] [==[Mbr.Extended085NotEmittedAsVolume]==] [==[Mbr.TornLinkKeepsPriorVolumes]==] [==[Mbr.TornLinkStatusReported]==] [==[Ebr.LinkType085WalksChain]==] [==[Ebr.LinkType085SameBasisAs05]==] [==[Slice.BoundsEnforced]==] [==[Slice.TailReadClampsAndZeroFills]==] [==[Slice.StartMustBeAligned]==] [==[Slice.RaggedTailAllowed]==] [==[Slice.GeometryRefreshAfterMediaChange]==] [==[Slice.GeometryRefreshRefusesInvalidatingChange]==] [==[Slice.CreateClampsOverlongDeclaredLength]==] [==[Slice.CreateStartBeyondDeviceStillRefused]==] [==[Slice.ExactFitNotClamped]==] [==[Slice.OrchestratorTargetClampScansRaggedTable]==] [==[DevMap.AddMergeSerialize]==] [==[DevMap.ModuleAttributionRecordedAndNewestWinsOnCoalesce]==] [==[DevMap.ModuleNotSerializedWireStaysTrio]==] [==[Storage.ParityDeltasListsNasExclusion]==] [==[Cached.HitMissAndBridging]==] [==[Cached.BoundsReject]==] [==[Cached.FailedBlocksAreNotCachedForever]==] [==[Cached.ScanCacheBudgetIsTheRegisteredDivergence]==] [==[DirectAlign.AlignedRequestIsTheFastPath]==] [==[DirectAlign.FourKMultipleNotSixtyFourKTakesTheEnvelope]==] [==[DirectAlign.UnalignedOffsetAndLengthEnvelope]==] [==[DirectAlign.MultiBlockEnvelopeCoversTheRequest]==] [==[DirectAlign.DeviceTailClampIsTheDocumentedException]==] [==[DirectAlign.OneMiBWriterChunkAtAlignedOffsetStaysDirect]==] [==[DirectAlign.OutOfBoundsAndEmptyRequestsFail]==] [==[DirectAlign.IssuedLengthAlwaysSixtyFourKMultipleExceptTail]==] [==[DirectAlign.UnifiedEnvelopeExhaustivePropertyBothDevices]==] [==[Retry.CleanReadPassesThrough]==] [==[Retry.FailModeReportsExhaustedAndMapsRange]==] [==[Retry.SkipModeZeroFillsOnlyBadSectors]==] [==[Retry.UnalignedBadRangeRoundsToWholeSectors]==] [==[Retry.BacktrackReadSalvagesPartialSector]==] [==[Retry.BacktrackOffReproducesLegacyExhausted]==] [==[Retry.BacktrackStaysAlignedOn4Kn]==] [==[Retry.BoundsAreEnforcedBeforeInner]==] [==[Retry.DeterministicFailureNotRetried]==] [==[Retry.GeometryForwarded]==] [==[Retry.ThreadStampAttributedToRecordedRange]==] [==[Retry.OwnerFallbackCoversUnstampedWorkerThreads]==] [==[Errmod.GuardSetsAndRestoresTheThreadStamp]==] [==[Pool.RunsAllItemsExactlyOnce]==] [==[Pool.AbortStopsWork]==] [==[Pool.ExternalCancelTokenAdapter]==] [==[Pool.ExceptionsNeverCrossThreadEdge]==] [==[Pool.WaitIdleAndPending]==] [==[Pool.CancelPoolWatcherDrainsQueueWithinOneTick]==] [==[Pool.CancelPoolWatcherNullTokenIsInert]==] [==[ParallelCarve.MatchesSingleThreaded]==] [==[ParallelRemnants.SameRecordsAsSequential]==] [==[NodeTree.AddRootsAndFiles]==] [==[NodeTree.HardLinkSharesProps]==] [==[NodeTree.ExtentDedupIndex]==] [==[NodeTree.ResetFreesClean]==] [==[Grading.MonotonicMaxNeverDowngrades]==] [==[Grading.Grade2Sticky]==] [==[Grading.DefaultGradeIsAverageAtEmit]==] [==[Grading.SizeFloorIs1]==] [==[Grading.EmailCategoryFastPath]==] [==[Grading.RefsAndBtrfsDeletedFilesGradeHigh]==] [==[Grading.RefsExistingStillHighAndSmallStillLow]==] [==[Grading.ExistingFilesHighDeletedNot]==] [==[Grading.SmallFileUnder1024High]==] [==[Grading.GaugePopulationIsMemberSet]==] [==[Grading.ScoreFormulaRoundsLikeOg]==] [==[Grading.QSDENotTheNtfsIndexCount]==] [==[Grading.NtfsDeviceIndexRuleUpgrades]==] [==[Grading.ExtensionTableGatesDeepValidation]==] [==[Grading.DotlessNamesSkipTwinDetection]==] [==[Grading.RefsDeletedFilesGradeHigh]==] [==[Grading.BtrfsDeletedFilesGradeHigh]==] [==[Grading.RefsExistingStillHigh]==] [==[Grading.TwinAddressFlagsBoth]==] [==[Grading.TableMembershipAndCount]==] [==[Grading.TwinKeyFat16StillShifts]==] [==[Grading.TwinKeyFat32DoesNotShift]==] [==[Grading.TwinKeyExfatDoesNotShift]==] [==[Grading.TwinKeyFat12DoesNotShift]==] [==[Grading.TwinKeyExt4NtfsNeverShift]==] [==[Grading.RegradeOnlyGrade1]==] [==[Grading.AssistantCountersMixedFixture]==] [==[Grading.SessionRoundTripPreservesGrade]==] [==[Grading.DeepReadZeroedFirstSectorGradesLow]==] [==[Grading.DeepRead0xFFHeadGradesLow]==] [==[Grading.DeepReadOwnLastSectorTailZeroGradesHigh]==] [==[Grading.DeepReadPrevSectorPaddingNotHigh]==] [==[Grading.DeepReadSector0StartFileNotDemotedByHead]==] [==[Grading.DeepReadShiftedKindSkippedLikeOgFailedRead]==] [==[Grading.DeepReadSkipsOverlapFlagged]==] [==[Grading.RegradeRunsFullTreeOnce]==] [==[Grading.DeepReadReaderAbsentChangesNothing]==] [==[Grading.RawTailZeroTicksForcedProbePopulation]==] [==[Grading.RawTailZeroSizeArmBoundary]==] [==[Grading.RawCountSemanticsRegistered]==] [==[Grading.TwinKeyExt2FamilyShifts]==] [==[Report.ContainsAssistantStatistics]==] [==[Report.ScanAnalysisCarriesPerFormat]==] [==[Report.EscapingStrictJson]==] [==[Report.SchemaShapeAndDeterminism]==] [==[Report.WriterOutcomesAndHashHonesty]==] [==[Report.MultiExtentSpannedVolumeRefused]==] [==[Report.DestFullFastFailStopsAttempting]==] [==[Report.DestFullPreCheckRefusesUpfront]==] [==[Report.ReadErrorOutcomeMarksTruncatedPartials]==] [==[Report.EmitsCiSchemaSample]==] [==[Report.FailedRowsCarryTreeClassification]==] [==[Report.FailedBytesSplitFromTotal]==] [==[Report.RawCategoryNameClassification]==] [==[Report.TreeClassIsAdditiveWithinSchemaV1]==] [==[Enumerate.ScopeDeltaRegisteredWithRevisitTrigger]==] [==[Enumerate.BusyDiskMetadataRetryShipsInQueryDisk]==] [==[Enumerate.ScopeContractDocumentedInHeader]==] [==[Enumerate.TrimChipOptInShipsInAbiAndCli]==] [==[Enumerate.SetupApiMerge]==] [==[Enumerate.DedupByInstancePath]==] [==[Enumerate.MergeWithEmptyInterfaceStackKeepsLoop]==] [==[Enumerate.MergeWithEmptyLoopTakesAllInterfaces]==] [==[Enumerate.DeviceDescriptorFieldsDecode]==] [==[Enumerate.DeviceDescriptorBoundedAndAbsent]==] [==[Enumerate.DeviceIdWwnDecode]==] [==[NtfsVolume.NonresidentAttrlistOnMft]==] [==[NtfsVolume.NestedAttrlistChase]==] [==[NtfsVolume.ResidentAdsCarriesContent]==] [==[NtfsVolume.AdsAttachesToFirstLink]==] [==[NtfsVolume.UsnJrnlHarvest]==] [==[NtfsVolume.UsnJrnlSparseV3Resync]==] [==[NtfsVolume.ShiftedHeaderSalvage]==] [==[NtfsVolume.EndToEndWalk]==] [==[NtfsVolume.OriginLcnCarriedFromFirstDataRun]==] [==[NtfsVolume.HardLinkSingleLinkMatchesOg]==] [==[NtfsVolume.HardLinkFanOutOptIn]==] [==[NtfsVolume.ParentAnchorIsMaskedFileNameParentRecord]==] [==[NtfsVolume.HardLinkFanOutClonesKeepShortName]==] [==[NtfsVolume.RecycleBinMapping]==] [==[NtfsVolume.RemnantScanFindsDetachedRecord]==] [==[NtfsVolume.ViabilityProbeUsesDataSize]==] [==[NtfsVolume.SiTimeSlotsMatchOgSurfacedSet]==] [==[NtfsVolume.ViabilityBoundaryExactFastPath]==] [==[NtfsVolume.TornRunlistKeepsPrefixAndGradesPartialRisk]==] [==[NtfsVolume.ExtensionRecordsNotCountedDamaged]==] [==[NtfsVolume.CompressedDataRecoversRealContent]==] [==[NtfsVolume.Lznt1MsXcaDocumentedVector]==] [==[NtfsVolume.RemnantScanReconstructsDataRunlist]==] [==[NtfsVolume.RemnantScanReconstructsResidentData]==] [==[NtfsVolume.RemnantScanCompressedStreamStaysMetadataOnly]==] [==[NtfsVolume.OverwrittenHonestModeKeepsAndGrades]==] [==[NtfsVolume.HardLinkDestroyRuleInteraction]==] [==[NtfsVolume.BootNoPow2RecordSizeAcceptedAtDecodedValue]==] [==[NtfsVolume.BootRecordSizeZeroDefaultsTo1024]==] [==[NtfsVolume.BootCibZeroDefaultsTo4096IndexBlocks]==] [==[NtfsVolume.BootCprZeroWithValidPrimaryDoesNotConsultBackup]==] [==[NtfsVolume.BootRecordShiftFormDecodesAt1024]==] [==[NtfsVolume.BootNegativeExponentOverflowRejected]==] [==[NtfsVolume.OddSectorRecordWalkScans]==] [==[NtfsVolume.OversizePositiveRecordOpensAndWalks]==] [==[NtfsVolume.BootSpcShiftDecodeConventionPinned]==] [==[NtfsVolume.BootUsableLargeRecordSizeKept]==] [==[NtfsVolume.RemnantReachableUnderTornDbr]==] [==[NtfsVolume.MirrorCoversRunlistSize]==] [==[NtfsVolume.MirrorResidentDataRejected]==] [==[NtfsVolume.UserFileNamedDollarNotHidden]==] [==[NtfsVolume.MainWalkKeepsOverwrittenDeleted]==] [==[NtfsVolume.GuidDumpPassRecoversReusedRecord]==] [==[NtfsVolume.OriginLcnCarriedOnGuidDumpRecord]==] [==[NtfsVolume.GuidDumpDestroyRules]==] [==[NtfsVolume.SizeZeroRescueFromMftCopy]==] [==[NtfsVolume.SizeZeroRescueSameNameMerge]==] [==[NtfsVolume.GuidDumpStrideIsRecordAligned]==] [==[NtfsVolume.GuidDumpLargeRecordParsedFully]==] [==[NtfsVolume.MainWalkKeepsSystemFlaggedUserFile]==] [==[NtfsVolume.OrphanIndxBlockHarvestsEntries]==] [==[NtfsVolume.IndxHiddenSystemEntryRejected]==] [==[NtfsVolume.IndxSlackNeverHarvested]==] [==[NtfsVolume.RemnantDirRecordHarvestsIndexRootChildren]==] [==[NtfsVolume.AdsDroppedInParityMode]==] [==[NtfsVolume.AdsEmittedByDefaultSuperset]==] [==[NtfsVolume.AttributesFromSIWhenAvailable]==] [==[NtfsVolume.AttributesFallBackToFileNameWhenSIAbsent]==] [==[NtfsVolume.AttributesPreserveOfflineBit]==] [==[NtfsVolume.ShortNameFieldRoundtrips]==] [==[NtfsVolume.UsnHarvestTrueDeleteReason0x80020000]==] [==[NtfsVolume.UsnHarvestRejectsCompressionChangeOnly]==] [==[NtfsVolume.UsnHarvestCapMinesTail]==] [==[NtfsVolume.DiskMapFreeCellsFromRealBitmap]==] [==[NtfsVolume.MultiHopAttrlistOgParityStopsAtDepth1]==] [==[NtfsVolume.DeletedAttrlistChaseFollowsChildrenAtDepth1]==] [==[NtfsVolume.TornPrimaryBackupBootScans]==] [==[NtfsVolume.TornSignatureWithIntactBackupStillFallsBack]==] [==[NtfsVolume.TornMftmirrLcnScansWithMirrorSkipped]==] [==[NtfsVolume.MftLcnProductFormBoundaryAndHardening]==] [==[NtfsVolume.TornPrimaryShapeBackupBootScans]==] [==[NtfsVolume.BackupBadFallsBackToRefusal]==] [==[NtfsVolume.BootFromBackupFlagged]==] [==[NtfsVolume.SpoofedOemRejectedEarly]==] [==[NtfsVolume.RecognitionNonPow2SpcDispatches]==] [==[NtfsVolume.RecognitionReservedBandNonzeroDeclinesAtDispatch]==] [==[NtfsVolume.RecognitionUnusedTailBandNonzeroDeclinesAtDispatch]==] [==[NtfsVolume.RecognitionUnsweptBandsStayAccepted]==] [==[NtfsVolume.RecognitionRecordSizeShapeGateDeclinesAtDispatch]==] [==[NtfsVolume.RecognitionIndexBlockShapeGateDeclinesAtDispatch]==] [==[NtfsVolume.RecognitionShapeGateZeroDefaultsPassAtDispatch]==] [==[NtfsVolume.RecycleBinScopeOutOfBinDollarFilesStayRaw]==] [==[NtfsVolume.LostDirGroupsOrphansByParentRecord]==] [==[NtfsVolume.LostDirNameIsRecordNumber]==] [==[NtfsVolume.TornMftRecord0BootstrapsFromMirror]==] [==[NtfsVolume.MirrorBootstrapSameRunlistAsIntactControl]==] [==[NtfsVolume.BothMftAndMirrorTornRefuses]==] [==[NtfsVolume.MirrorBootstrapFlaggedInOpenDiagnostics]==] [==[FatEngine.FreeMapExcludesVirtualClustersAndMatchesFAT]==] [==[FatEngine.Fat16EndToEnd]==] [==[FatEngine.LfnIllegalCharFallsBackTo83]==] [==[FatEngine.LfnValidNameUnchanged]==] [==[FatEngine.LfnCorruptFallsBackToShort]==] [==[FatEngine.ShortNameOemBytesDecoded]==] [==[FatEngine.ShortNameAsciiUnchanged]==] [==[FatEngine.ShortNameKaniMarkerPathUnchanged]==] [==[FatEngine.NtResCaseFoldFourWay]==] [==[FatEngine.NtResFoldLeavesLfnCaseUntouched]==] [==[FatEngine.Fat32RootChainAndBackupBoot]==] [==[FatEngine.Fat16HasNoBackupBootFallback]==] [==[FatEngine.Fat32BackupBootFallback]==] [==[FatEngine.ChainCycleIsCut]==] [==[FatEngine.Fat12ChainsCanonicalEocAndBad]==] [==[FatEngine.FatOriginClusterCarriedForChainlessDeleted]==] [==[FatEngine.ParentAnchorCarriesContainingDirCluster]==] [==[FatEngine.DeletedLfnFragmentOrder]==] [==[FatEngine.DeletedLfnRealWorldChecksumRecoversFullName]==] [==[FatEngine.DeletedLfnSingleSlotChecksumRecovery]==] [==[FatEngine.DeletedLfnTornChecksumKeepsHonestFallback]==] [==[FatEngine.LfnDeletedChecksumBijection]==] [==[FatEngine.DeletedUnlinkedChainGetsContiguousRun]==] [==[FatEngine.TruncatedVolumeOverflowChainStance]==] [==[FatEngine.DeletedUnlinkedChainTruncatesAtReusedCluster]==] [==[FatEngine.DeletedLinkedChainStillPrefersFatTruth]==] [==[FatEngine.Fat32EocMask24AndAlternateFamiliesEndClean]==] [==[FatEngine.Fat16BadClusterStillCanonical]==] [==[FatEngine.LargeFat32TableAccepted]==] [==[FatEngine.SmallFat16TableDeclined]==] [==[FatEngine.MirrorShiftedCopyRejected]==] [==[FatEngine.MirrorCrossLinkRejected]==] [==[FatEngine.MirrorOnlyScanFat1Unreadable]==] [==[FatEngine.CrossLinkedFat1RejectedAndFat2Used]==] [==[FatEngine.MediaByteCanonicalEnforcedFat16]==] [==[FatEngine.InvalidDateYieldsZeroFileTime]==] [==[FatEngine.InvalidMonthYieldsZeroFileTime]==] [==[FatEngine.InvalidDayYieldsZeroFileTime]==] [==[FatEngine.ValidDateYieldsCorrectFileTime]==] [==[FatEngine.TimeWordHour24YieldsZeroFileTime]==] [==[FatEngine.TimeWordMinute60YieldsZeroFileTime]==] [==[FatEngine.TimeWordSecondField30YieldsZeroFileTime]==] [==[FatEngine.ValidTimeWordBoundaryUnchanged]==] [==[FatEngine.InvalidTimeStampsPersistAsZeroInTree]==] [==[FatEngine.FlavorBoundary65524IsFat32]==] [==[FatEngine.BootStringGateDeclinesTornStringVolume]==] [==[FatEngine.BootStringCaseInsensitiveAccepted]==] [==[FatEngine.Fat16LowRootEntriesDeclined]==] [==[FatEngine.Fat16RootEntriesNotSectorMultipleDeclined]==] [==[FatEngine.NumFatsThreeDeclined]==] [==[FatEngine.SpcDomainMatchesOgSet]==] [==[FatEngine.FatBps1536Accepted]==] [==[FatEngine.FatBps32768CeilingAndNonMultipleRejected]==] [==[FatEngine.FatZeroNumFatsRejected]==] [==[FatEngine.FatClusterCountOverflowRejected]==] [==[FatEngine.DateSanityRejectedInStrictMode]==] [==[FatEngine.BadPrimaryServedWithoutMirrorConsult]==] [==[FatEngine.PrimaryFreeStillConsultsMirror]==] [==[FatEngine.RecyclePairingRestoresOriginalName]==] [==[FatEngine.RecycleLegacyRoots]==] [==[FatEngine.NonRecycleDollarFilesStayRaw]==] [==[FatEngine.ReadVolumeLabelDecodesBootSector]==] [==[FatEngine.ReadVolumeLabelRejectsLabelLessAndNonFat]==] [==[FatEngine.ClusterOffsetBeyond2TiB]==] [==[FatEngine.DeletedFirstCharPlaceholderIsHash]==] [==[FatEngine.DeletedLfnChecksumRecoversOriginalByte]==] [==[FatEngine.Date2107ConvertsFullyNoClamp]==] [==[FatEngine.StringFat16LowClusterCountScansAsFat16]==] [==[FatEngine.StringFat32UnderBracketScansAsFat32]==] [==[FatEngine.HybridStringFat16MediaFat32ReflavorsToFat32]==] [==[FatEngine.Fat2ReadWinsOverFat1]==] [==[FatEngine.ConsistentVolumeUnreflavored]==] [==[FatEngine.StringlessSmallVolumeStillBracketsFat12]==] [==[FatEngine.StringGateRejectsShapelessVolume]==] [==[FatEngine.TimeWordOutOfRangeYieldsZeroTimestamp]==] [==[FatEngine.ValidTimeWordBoundaryConvertsUnchanged]==] [==[FatEngine.InvalidTimeStampsPersistAsZero]==] [==[FatEngine.InvalidTimeWordAccessSlotStaysReal]==] [==[FatEngine.DentryAttrReservedBitsStopWalk]==] [==[FatEngine.DentryErasedPatternQuadStopsWalk]==] [==[FatEngine.DentryHistogramEleventhRepeatStopsWalk]==] [==[FatEngine.DentryHistogramTenRepeatsStillEmit]==] [==[FatEngine.DentryImpureLabelStopsWalk]==] [==[FatEngine.PureVolumeLabelSkippedWalkContinues]==] [==[FatEngine.SizeChainXorSkippedButWalkContinues]==] [==[FatEngine.DirSizeAndHighClusterSkipped]==] [==[FatEngine.Date2107EndToEndFileTimeIsReal]==] [==[FatEngine.AccessSlotStampsDateAtMidnightWhenTimeWordInvalid]==] [==[FatChain.RunCoalescingDetectsOverlap]==] [==[FatChain.CycleGuardPerf]==] [==[ExfatEngine.LostAnchorBackupBootRebasedToTrueStart]==] [==[ExfatEngine.HardeningBoundsPinned]==] [==[ExfatEngine.IntactPrimaryNotShifted]==] [==[ExfatEngine.MediaWordsAbsentNoShift]==] [==[ExfatEngine.ShiftedBaseBelowZeroRefused]==] [==[ExfatEngine.TimestampsMatchOgLocalAsUtc]==] [==[ExfatEngine.TimestampsZeroTzPassThrough]==] [==[ExfatEngine.TimestampsMatchExplorerDisplay]==] [==[ExfatEngine.TimestampsWestOffsetAlsoIgnored]==] [==[ExfatEngine.OriginClusterCarriedFromStreamDentry]==] [==[ExfatEngine.FreeMapExcludesVirtualClustersAndMatchesBitmap]==] [==[ExfatEngine.TornBitmapReadYieldsNoFreeMap]==] [==[ExfatEngine.EndToEnd]==] [==[ExfatEngine.UpcaseTableChecksumValidated]==] [==[ExfatEngine.UpcaseLengthSanity]==] [==[ExfatEngine.FreeMapProducerMatchesSpecBitMapping]==] [==[ExfatEngine.SetChecksumMismatchGradesExcellentInOgProfile]==] [==[ExfatEngine.SetChecksumMismatchMarksDamaged]==] [==[ExfatEngine.InvalidFirstClusterRejected]==] [==[ExfatEngine.EocFamilyTerminatesChain]==] [==[ExfatEngine.FreeEntryContiguousSalvage]==] [==[ExfatEngine.BootGatesMatchOg]==] [==[ExfatEngine.UpcaseAlgorithmMatchesRealMkfsExfat]==] [==[ExfatEngine.LargeFatFullChains]==] [==[ExfatEngine.LargeFatDirectoryWalk]==] [==[ExfatEngine.StreamReservedPartiallyNonzeroPassesOgByteOr]==] [==[ExfatEngine.StreamReservedAllBytesNonzeroStillDamaged]==] [==[ExfatEngine.StreamReserved1ByteStrictlyZero]==] [==[ExfatEngine.DeletedSetChecksumRejectsTornSet]==] [==[ExfatEngine.DeletedSetOracleReassembly]==] [==[ExfatEngine.LiveCompleteSetBadChecksumUngraded]==] [==[ExfatEngine.AppleDoublePrefixSunk]==] [==[ExfatEngine.RegularDotFilesNotSunk]==] [==[ExfatEngine.EmptyFileSuppressedLikeOg]==] [==[ExfatEngine.GarbageTimestampRejected]==] [==[ExfatEngine.SecondaryCountCapped]==] [==[ExfatEngine.BadNameFlagsRejected]==] [==[ExfatEngine.FileReservedNonzeroRejected]==] [==[ExfatEngine.ValidBoundaryTimestampsStillPass]==] [==[ExfatEngine.NameUtf16TableInvalidCharRejected]==] [==[ExfatEngine.NameUtf16TableAllowedCharAccepted]==] [==[ExfatEngine.RecyclePairingMirrored]==] [==[Ext4Engine.ExtentTreeDepth3]==] [==[Ext4Engine.SuperblockValidation]==] [==[Ext4Engine.SbBandsPinned]==] [==[Ext4Engine.EndToEndWalk]==] [==[Ext4Engine.BadMagicRejected]==] [==[Ext4Engine.UnixTimeConversion]==] [==[Ext4Engine.SizeExceedsBlocksDamagedDirectoryStaysUnflagged]==] [==[Ext4Engine.LegacyAclDirSizeClean]==] [==[Ext4Engine.Post2038FileTimeConversionStays64Bit]==] [==[Ext4Engine.SizeHighFoldIsRegularOnlyNotTheOgUnconditionalDefect]==] [==[Ext4Engine.UnwrittenExtentLenDecodedKernelCorrect]==] [==[Ext4Engine.SizeExceedsBlocksSetsDamagedFlagConvergent]==] [==[Ext4Engine.DamagedFlagCleanFilesCarryNoBit]==] [==[Ext4Engine.DamagedFlagSessionRoundTrip]==] [==[Ext4Engine.JournalSalvage]==] [==[Ext4Engine.InodeSizeOffsetIsSpecTrue]==] [==[Ext4Engine.JournalUncommittedTailSurfaces]==] [==[Ext4Engine.FamilyKindStampsPerDecodedAnalyserRule]==] [==[Ext4Engine.JournalUnreferencedDirBlockSalvaged]==] [==[Ext4Engine.JournalFirstBlockNotJsbNoSalvage]==] [==[Ext4Engine.CorruptRecLenSkipsEntryNotBlock]==] [==[Ext4Engine.InodeZeroHoleContinues]==] [==[Ext4Engine.FileTypeGateSkipsInvalid]==] [==[Ext4Engine.NonAsciiNamesDecodedNotByteLifted]==] [==[Ext4Engine.MetaBgVolumeRecoversAllGroups]==] [==[Ext4Engine.MetaBgDeletedSweepReachesDistributedGroups]==] [==[Ext4Engine.MetaBgDescriptorBlockReadOnce]==] [==[Ext4Engine.MetaBgFirstMetaBgFrontRegion]==] [==[Ext4Engine.MetaBgSuperblockFieldsParsed]==] [==[Ext4Engine.BackupSuperblockRescueWhenPrimaryDestroyed]==] [==[Ext4Engine.BackupRescueFallsBackToBackupGdt]==] [==[Ext4Engine.BackupRescueWholeDiskRebase]==] [==[Ext4Engine.BadMagicWithoutBackupsStillRejected]==] [==[Ext4Engine.JournalSalvagedDirReparented]==] [==[Ext4Engine.JournalUnreferencedOrphanFlatWithRealName]==] [==[Ext4Engine.TwoPassJournalClaimInteraction]==] [==[Ext4Engine.CatchBufferBacktrackSalvage]==] [==[Ext4Engine.BacktrackOffZeroFillsTheWindow]==] [==[Ext4Engine.ZeroFillOnlyWhenBacktrackAlsoFails]==] [==[Ext4Engine.NanosecondsPreservedFromExtraField]==] [==[Ext4Engine.InvalidUtf8NameCarriesRawBytes]==] [==[Ext4Engine.PoisonDoubleIndirectBoundedBySize]==] [==[Ext4Engine.PoisonTripleIndirectBoundedBySize]==] [==[Ext4Engine.StaleIndirectCancelResponsive]==] [==[Ext4Engine.ContiguousPhysicalBlocksMergedToSingleExtent]==] [==[Ext4EngineM144.Large64BitVolumeAbove8MiBGdtOpens]==] [==[Ext4EngineM144.CorruptHugeGroupCountStillRefused]==] [==[Ext4EngineM143.JournalInodeImageSurfacesInodeRecord]==] [==[Ext4EngineM143.JournalInodeImageZeroSizeSkipped]==] [==[Ext4EngineM143.JournalInodeImageNoExtentsFlagSkipped]==] [==[Ext4EngineM143.JournalInodeImageBadExtentMagicSkipped]==] [==[Ext4EngineM143.JournalInodeImageZeroEhEntriesSkipped]==] [==[Ext4EngineM143.JournalDirPairTakesPrecedence]==] [==[HfsPlus.VolumeHeaderValidation]==] [==[HfsPlus.VhFreeBlocksAboveTotalRejected]==] [==[HfsPlus.VhJournalInfoBlockAboveTotalRejected]==] [==[HfsPlus.VhTornAllocForkStillValidates]==] [==[HfsPlus.VhFreeBlocksEqualTotalAccepted]==] [==[HfsPlus.EndToEndWalk]==] [==[HfsPlus.DecmpfsInlineAndRsrcContentRecovered]==] [==[HfsPlus.DecmpfsDisabledLeavesEmptyShell]==] [==[HfsPlus.MacTimeConversion]==] [==[HfsPlus.BadSignatureRejected]==] [==[HfsPlus.LeafNodeLayoutMatchesOnDiskFormat]==] [==[HfsPlus.DotfileAndPrivateDataFiltered]==] [==[HfsPlus.PrdfRecordFolderized]==] [==[HfsPlus.RecordBit29SelectsResourceFork]==] [==[HfsPlus.LargeBlockSizeAccepted]==] [==[HfsPlus.ExtentsOverflowRecordResolvesFragmentedFork]==] [==[HfsPlus.JournalHarvestEmitsDeletedRecords]==] [==[HfsPlus.JournalDifferentialDifferentFileIdKept]==] [==[HfsPlus.JournalBadHeaderGates]==] [==[HfsPlus.JournalWrapHandled]==] [==[HfsPlus.JournalHarvestDisabledByFlag]==] [==[HfsPlus.JournalSameKeyDifferentFileIdBothKept]==] [==[HfsPlus.JournalAttrDecmpfsFallbackFeedsEmptyForkFile]==] [==[HfsPlus.JournalRequiresVolumeJournaledBit]==] [==[HfsPlus.JournalInfoBlockHasNoMagicField]==] [==[HfsPlus.JournalNodeImageRejectsInconsistentOffsetTable]==] [==[HfsPlus.JournalFallbackSalvageWhenHeaderInvalid]==] [==[HfsPlus.JournalFallbackRequiresNodeShapedStart]==] [==[HfsPlus.ZeroLogicalSizeFallsBackToTotalBlocks]==] [==[Btrfs.MtimeNsecFoldsLikeTheOg]==] [==[Btrfs.SuperblockValidation]==] [==[Btrfs.NonPow2NodesizeRejectedRegistered]==] [==[Btrfs.EndToEndWalk]==] [==[Btrfs.TwoSubvolumesRootSeparately]==] [==[Btrfs.SubvolumeNamedFromRootRef]==] [==[Btrfs.FileAttributeNormalBit]==] [==[Btrfs.BadMagicRejected]==] [==[Btrfs.PackedDirItemsInOneItem]==] [==[Btrfs.MirrorSelectionNewestGeneration]==] [==[Btrfs.InternalKeyPtrWalk]==] [==[Btrfs.InlineAndCompressedExtents]==] [==[Btrfs.NonAsciiNamesDecodedNotByteLifted]==] [==[Btrfs.Latin1NameRoundTripsRawBytes]==] [==[Btrfs.XattrDosAttribApplied]==] [==[Btrfs.UserXattrRecordsEmitted]==] [==[Btrfs.CompressionPropertyTagged]==] [==[Btrfs.Raid10GeometryGate]==] [==[Btrfs.Raid1RotatingMirrorReads]==] [==[Btrfs.Raid0StripeMathSplitsAtStripeBoundaries]==] [==[Btrfs.DefaultDirItemNonSubvolumeLocationFallsBack]==] [==[Btrfs.CacheRoundTripAndInvalidation]==] [==[Btrfs.CacheDisabledByDefault]==] [==[Btrfs.CacheWarmScanSkipsChunkDerivation]==] [==[Btrfs.CacheStaleGenerationDiscardedOnVolumeRewrite]==] [==[Btrfs.EaDirSubtreeSuppressed]==] [==[Btrfs.EaDirExactMatchOnly]==] [==[Btrfs.EaDirNodeItselfEmitted]==] [==[Btrfs.EaDirNotInFilterfolderPools]==] [==[Zfs.EndToEndWalk]==] [==[Zfs.ChildDirZapReadsKernelOffset32]==] [==[Zfs.OriginObjNotTreatedAsChildZap]==] [==[Zfs.LabelPositionsRecoveredFromTail]==] [==[Zfs.NewestTxgAcrossLabelsWins]==] [==[Zfs.RingWindowDeletedFileRecalled]==] [==[Zfs.RingWindowUnchangedEntriesEmitOnce]==] [==[Zfs.RingWindowModifiedFilePriorVersionNotReemitted]==] [==[Zfs.RingTornNewestStateFallsBackToOlder]==] [==[Zfs.ChecksumFailDropsAndRetriesNextDva]==] [==[Zfs.StripedPoolForeignVdevFallbackRead]==] [==[Zfs.MirrorSecondMemberScan]==] [==[Zfs.SaRegistryDrivenLayout2]==] [==[Zfs.SaRegistryAbsentFallsBackToLayout1]==] [==[Zfs.BadMagicRejected]==] [==[Zfs.ZleRoundTrip]==] [==[Zfs.CodecMapping]==] [==[Zfs.CodecRoundTrips]==] [==[Zfs.TruncatedSourcesRejectedNotOverread]==] [==[Zfs.NonAsciiNamesDecodedNotByteLifted]==] [==[Zfs.LabelRetryBackoff]==] [==[Zfs.LabelRetryGivesUpBounded]==] [==[Zfs.StaticMediaNeverRetries]==] [==[Zfs.RingWindowRenamedEntryRecalledUnderBothNames]==] [==[Zfs.TrashInfoSidecarChildrenSuppressed]==] [==[RefsDetect.FindsMsbPages]==] [==[RefsDetect.CheckpointVariant]==] [==[RefsDetect.IdentificationFallback]==] [==[RefsDetect.PartialSignaturePageDetectedAndFlagged]==] [==[RefsDetect.FullSignaturePageNotFlaggedPartial]==] [==[RefsDetect.NonRefsRejected]==] [==[RefsDetect.PartialSignaturePageDetected]==] [==[Ldm.ParsesDatabaseAndGraph]==] [==[Ldm.ResolvesSimpleVolume]==] [==[Ldm.SimpleVolumeDeviceReads]==] [==[Ldm.StripedVolumeInterleave]==] [==[Ldm.CompositeVblkRecord]==] [==[Ldm.NonLdmDiskRejected]==] [==[Ldm.Raid5LeftSymmetricReads]==] [==[Ldm.Raid5ReconstructsMissingColumn]==] [==[Ldm.Raid5TwoMissingColumnsUnrecoverable]==] [==[Ldm.MirroredReadFailover]==] [==[Ldm.MirroredBothCopiesFail]==] [==[Ldm.SpannedConcatenation]==] [==[Ldm.GptDynamicDiskCanonicalGuidsParse]==] [==[Ldm.GptDynamicDiskFabricatedGuidsRejected]==] [==[Ldm.GptDynamicDiskTableClassifiesLdmEntries]==] [==[Ldm.GptDynamicDiskRealCanonicalGuidsParse]==] [==[Ldm.GptPathFindsMetadata4Kn]==] [==[Ldm.TocblockAtConfigPlus0]==] [==[Ldm.TocblockPrefersPlus2OnConflict]==] [==[Ldm.TocblockTailCopies]==] [==[Ldm.OversizeConfigRejected]==] [==[Ldm.ConfigSizeExactlyOneMebibyteAccepted]==] [==[Ldm.ZeroInterleaveDefaultsTo64K]==] [==[LostDiscovery.FindsScoresAndRejectsNoise]==] [==[LostDiscovery.OverlapClassification]==] [==[LostDiscovery.TwoHealthyDeletedPartitionsBothSurface]==] [==[LostDiscovery.DamagedBpbFallbackMustNotSwallowPeer]==] [==[LostDiscovery.Ext4SuperblockAnchorsDeletedPartition]==] [==[LostDiscovery.Ext4BlockGroupCopyNotACandidateStart]==] [==[LostDiscovery.HfsPlusVolumeHeaderAnchorsDeletedPartition]==] [==[LostDiscovery.BtrfsSuperblockAnchorsDeletedPartition]==] [==[LostDiscovery.ZfsLabelAnchorsDeletedPartition]==] [==[LostDiscovery.TailBoundUsesCeiledSectorCount]==] [==[LostDiscovery.RefsBootAnchorsDeletedPartition]==] [==[LostDiscovery.SuperblockNoiseRejected]==] [==[LostDiscovery.LostBitlockerPartitionDiscovered]==] [==[LostDiscovery.LostBitlockerOemNoise]==] [==[LostDiscovery.LostBitlockerInsideExtendedRegionStillFound]==] [==[LostDiscovery.FatBootWithOgGatesBecomesCandidate]==] [==[LostDiscovery.Fat32ExtentUsesDataAreaForContainment]==] [==[LostDiscovery.LostCandidateReportsDataAreaOffset]==] [==[LostDiscovery.FileRichCandidateOutranksCleanBoot]==] [==[LostDiscovery.TableMatchBonus500]==] [==[LostDiscovery.OverLargeDuplicateRescoredToFront]==] [==[LostDiscovery.ZeroSectorSizeFallsBackTo512]==] [==[LostPart.ForgedOemNoMftEvidenceRejected]==] [==[LostPart.TornPrimaryMftMirrorEvidenceScored]==] [==[LostPart.HealthyCandidateMftEvidenceUnchanged]==] [==[LostFatGates.Spc256Rejected]==] [==[LostFatGates.NumFatsZeroRejectedThreeAcceptedByDeepGates]==] [==[LostFatGates.Fat32NameStringTornYieldsNoCandidate]==] [==[LostFatGates.Fat12SmallFatProbe]==] [==[LostFatGates.Fat12ProbeSpfBoundIsExclusiveAt200]==] [==[LostFatGates.Fat32GateStringIsCaseSensitive]==] [==[LostScoring.MatchesLogRows]==] [==[MountPoints2Decode.ParsesCleanPlacement]==] [==[MountPoints2Decode.LowercaseHexAccepted]==] [==[MountPoints2Decode.ShortBlobRejected]==] [==[MountPoints2Decode.MissingDelimiterRejected]==] [==[MountPoints2Decode.MisalignedStartRejected]==] [==[MountPoints2Decode.NonHexFieldRejected]==] [==[MountPoints2Decode.TruncatedHexStillParsesPrefix]==] [==[MountPoints2Decode.SectorZeroIsTheFailureSentinel]==] [==[MountPoints2Walk.ParsesPlacementsFromSyntheticHive]==] [==[MountPoints2Walk.AbsentKeyYieldsZeroPlacements]==] [==[MountPoints2Walk.ZeroBpsRefused]==] [==[MountPoints2Walk.CaseInsensitiveKeyPath]==] [==[RegisterChannel.FindsStrideMissedPartition]==] [==[RegisterChannel.ThreeSourceDedupSingleCandidate]==] [==[RegisterChannel.SweepProposalWinsProvenance]==] [==[RegisterChannel.UnprobeablePlacementDropped]==] [==[RegisterChannel.OutOfBoundsPlacementDropped]==] [==[RegisterChannel.RegisterRecordsAreRevalidated]==] [==[RegisterChannel.EmptyHistoryIsThePreFixShape]==] [==[Sha256.NistVectors]==] [==[Sha256.StreamedMatchesOneShot]==] [==[Sha256.Id128Truncation]==] [==[AesXts.Aes128Fips197KnownAnswer]==] [==[AesXts.Aes256Fips197KnownAnswer]==] [==[AesXts.Xts128CrossImplementationVector]==] [==[AesXts.Xts256CrossImplementationVector]==] [==[AesXts.Xts256WholeSectorVector]==] [==[AesXts.WeakKeyHalvesRejected]==] [==[AesXts.RoundTripVariedLengths]==] [==[AesXts.DistinctTweaksGiveDistinctCiphertexts]==] [==[DecryptingDevice.ReadsPlaintextThroughWrapper]==] [==[DecryptingDevice.UnalignedAndCrossSectorReads]==] [==[DecryptingDevice.InnerErrorsPropagateVerbatim]==] [==[DecryptingDevice.BadKeyMaterialCreateFails]==] [==[BitLockerStack.EncryptedVolumeScansOnlyThroughDecryptDevice]==] [==[Dict.ShippedTableMatchesCorpusShape]==] [==[Dict.ShippedTableKeepsCorpusDuplicates]==] [==[Dict.ParseDictionaryGrammar]==] [==[Dict.ParseToleratesLfAndEmptyLines]==] [==[Dict.ParseRoundTripsTheCorpusGrammar]==] [==[Dict.ParseRejectsBadGrammar]==] [==[Dict.TryStopsAtFirstSuccessInShippedOrder]==] [==[Dict.UserCandidatesFollowShippedDictionary]==] [==[Dict.AllFailReportsEveryAttempt]==] [==[Dict.NullAttemptFunctionIsANoOp]==] [==[Dict.AttemptsLoggedThroughSeamWithoutPasswordMaterial]==] [==[ResumeRevalidation.ResumeDropsStaleCandidateAfterRelayout]==] [==[ResumeRevalidation.ResumeKeepsValidCandidate]==] [==[ResumeRevalidation.DiskLayoutRecordRoundTrip]==] [==[ResumeRevalidation.LayoutChangedFlagged]==] [==[ResumeRevalidation.SizeSourceFlagRoundTrip]==] [==[ResumeRevalidation.CandidateGeometryRoundTrip]==] [==[ResumeRevalidation.ArbiterStartOffDiskDropsAndUnknownKeeps]==] [==[ResumeRevalidation.RoundTripKeepsDataAreaExtent]==] [==[ResumeRevalidation.V3SessionResumeReDerivesDataArea]==] [==[ResumeRevalidation.RestoredWindowMatchesFreshDiscovery]==] [==[ResumeRevalidation.EnrichedExtentIsIdempotentAcrossReSave]==] [==[Text.AsciiPassesThrough]==] [==[Text.LatinAccentDecodes]==] [==[Text.CjkDecodesToCodePoints]==] [==[Text.AstralPlaneBecomesSurrogatePair]==] [==[Text.OverlongFormsRejectedAsReplacement]==] [==[Text.SurrogateHalvesRejected]==] [==[Text.BeyondUnicodeRejected]==] [==[Text.InvalidLeadBytesResyncOneByte]==] [==[Text.TruncatedTailBecomesReplacement]==] [==[Text.MaximalSubpartReplacement]==] [==[Text.EmbeddedNulDecodes]==] [==[Text.RoundTripThroughNodeUtf8]==] [==[Text.InvalidByteLosslessMapping]==] [==[Text.LosslessRoundTripRecoversRawBytes]==] [==[Text.ZfsDecoderStaysReplacement]==] [==[Text.WideToUtf8EncodesNonAscii]==] [==[Text.WideToUtf8SurrogatePairAndLoneHalf]==] [==[Text.CliFilterUtf8MatchesDecodedNames]==] [==[CarveEngine.GrammarValidationAndSpecificity]==] [==[CarveEngine.CarvesMidSectorStart]==] [==[CarveEngine.SecondPassFullyDeduped]==] [==[CarveEngine.VideoPostProcessorHookInvoked]==] [==[CarveEngine.HookDegradesWhenModuleAbsent]==] [==[CarveEngine.RegistryF8Gate]==] [==[CarveEngine.PstAndRar5Grammar]==] [==[CarveEngine.ExtendedFormatsGrammar]==] [==[CarveEngine.PdfEofLastWinsAndFrameFloors]==] [==[CarveEngine.RawSignLayerHeaderOnlyAndSharedMagic]==] [==[CarveEngine.RawSignTailPairWithinFormat]==] [==[CarveEngine.UnreadableWindowNotCountedAsScanned]==] [==[CarveEngine.ZipCdAnchorCarvesDestroyedHead]==] [==[CarveEngine.ZipIndexlessLfhChainSalvage]==] [==[CarveEngine.ZipSalvageAbsorbsDataSectors]==] [==[CarveEngine.Zip64ExtraFieldContinuesChain]==] [==[CarveEngine.ZipCleanArchiveNotInflatedByZeroSlack]==] [==[CarveEngine.BeTiffAndOrfVariantsCarve]==] [==[CarveEngine.OverlapScorePrefersHigherConfidence]==] [==[CarveEngine.TiffVsRawTiebreak]==] [==[CarveEngine.PstUnknownVersionAcceptedAtFloor]==] [==[CarveEngine.JpegSof0PrecisionGate]==] [==[CarveEngine.SlicedDeviceSecondPassFullyDedupedAndAbsolute]==] [==[CarveEngine.CarvedNodesAreDeleted]==] [==[CarveEngine.RawSignCarvedNodesAreDeleted]==] [==[CarveEngine.DeletedOnlyQueryIncludesCarvedFiles]==] [==[CarveEngine.CarvedNodesCarryScanTimestamp]==] [==[CarveEngine.RawSignCarvedNodesCarryScanTimestamp]==] [==[CarveEngine.ScanTimestampSurvivesSessionRoundTrip]==] [==[CarveEngine.ZipInnerEntryInventoryEmitted]==] [==[CarveEngine.ZipIndexlessEntryInventory]==] [==[CarveEngine.Rar4InnerEntryInventory]==] [==[CarveEngine.ProvenanceChecksumAndAnchorsPersist]==] [==[CarveEngine.TailAnchorCarriedOnTailPairedHits]==] [==[CarveEngine.SqliteRealHeaderByteOrder]==] [==[CarveEngine.SqliteInvalidEncodingRejects]==] [==[CarveEngine.SqliteLittleEndianShapeRejects]==] [==[CarveEngine.SqliteFcpeventSubtypeRelabeled]==] [==[CarveEngine.SqliteGenericLabelUnchangedWithoutMarker]==] [==[CarveEngine.BitmapSkipsUsedClusters]==] [==[CarveEngine.NoBitmapCarvesEverything]==] [==[CarveEngine.FormatDisabledSkipsSignature]==] [==[CarveEngine.FormatDisabledRawSignsToo]==] [==[CarveEngine.FilterExemptSetAlwaysOn]==] [==[CarveEngine.FilterExemptSetCoversTypedRows]==] [==[CarveEngine.AllFormatsEnabledByDefault]==] [==[CarveEngine.WindowServingEliminatesPerCandidateRereads]==] [==[CarveEngine.WindowServingServesStraddlingReadsCorrectly]==] [==[CarveEngine.Mp4NamedByDurationPrefix]==] [==[CarveEngine.MovNamedByDurationPrefix]==] [==[CarveEngine.MovV1MvhdDurationRead]==] [==[CarveEngine.ReconstructedMultiVolumeGrouping]==] [==[CarveEngine.ReconstructedUnattributedPositionStaysAtRoot]==] [==[CarveEngine.ReconstructedSingleDefaultGroupName]==] [==[CarveEngine.OgNamingUniversalExtCounter]==] [==[CarveEngine.OgNamingJpgSofDimensions]==] [==[CarveEngine.JpegExifPrefixCappedAtThirtyNine]==] [==[CarveEngine.OgNamingJpgExifModelAndSoftware]==] [==[CarveEngine.OgNamingMp3Id3Title]==] [==[CarveEngine.OgNamingPdfCreatorAndDatePrefixes]==] [==[CarveEngine.OgNamingDocxCreatedTimestamp]==] [==[CarveEngine.OgNamingDocxCreatorComposesWithCreated]==] [==[CarveEngine.OgNamingCounterKeyTwoClasses]==] [==[CarveEngine.OgNamingSessionWideCounters]==] [==[CarveEngine.OgNamingWavInamTitle]==] [==[CarveEngine.OgNamingCounterPast99999Unpadded]==] [==[CarveEngine.OgNamingZipFirstCentralDirectoryEntry]==] [==[CarveEngine.FileNnnnFallbackRemoved]==] [==[CarveEngine.ZeroSectorSkipStats]==] [==[CarveEngine.UserSignatureProbedAtStride]==] [==[CarveEngine.UserSignatureYieldsToBuiltInAtSamePosition]==] [==[CarveEngine.UserSignYieldsToRawSignTableAtSamePosition]==] [==[CarveEngine.DisabledFormatGatesUserSignLayer]==] [==[CarveEngine.UserSignFilesCountInCarvedFound]==] [==[CarveEngine.WindowBufferReusedByValidatorChecksumAndHash]==] [==[CarveEngine.BoundaryStraddlingCandidateSplitsReadsCorrectly]==] [==[CarveEngine.SkipsFsAllocatedRegions]==] [==[CarveEngine.StructuredBoundaryPartialOverlap]==] [==[CarveEngine.OccupancyGateJumpsWholeInterval]==] [==[Carve.StatsPerFormatTally]==] [==[Carve.Mp4RealFtypIsom]==] [==[Carve.MovRealFtypQt]==] [==[Carve.HeicRealFtypHeic]==] [==[Carve.M4aRealFtyp]==] [==[Carve.MovMoovWithoutFtyp]==] [==[Carve.RegistrySelfProbeReachesEveryFormat]==] [==[Carve.X3fExtentIsSecdPlusFixedTail]==] [==[Carve.X3fNoSectionDirectorySalvagesWalkedPrefix]==] [==[Carve.X3fGarbageCountIsIrrelevantToExtent]==] [==[Carve.S007CommonFormatsCarve]==] [==[Carve.S007ValidatorsRejectNoise]==] [==[Carve.S007TruncatedSalvageSemantics]==] [==[Carve.S007NicheBatch1Carve]==] [==[Carve.S007NicheBatch1RejectsNoise]==] [==[Carve.S007M081CiwMrmLowdocHlpOgGrammar]==] [==[Carve.S007NicheBatch2School1Carve]==] [==[Carve.M090DmgKolyFooterExtent]==] [==[Carve.M090DmgKolyUnalignedSalvage]==] [==[Carve.M090DmgKolyZeroTailGateAndCap]==] [==[Carve.M084AccdbSecondSignatureCarves]==] [==[Carve.M091MdbPageChainWalk]==] [==[Carve.S007NicheBatch2School1RejectsNoise]==] [==[Carve.WmfPlaceableOgGrammarCarves]==] [==[Carve.WmfPlaceableOldCoreGrammarRejected]==] [==[Carve.WmfPlaceableTwentyByteVariantRejected]==] [==[Carve.WmfStdOgGatesAcceptNonCanonicalHeaders]==] [==[Carve.MpegTs188PacketStreamAccepted]==] [==[Carve.M2Ts192PacketStreamAccepted]==] [==[Carve.TsBelowMinimumExtentRejected]==] [==[Carve.TsSyncLossCommitsRun]==] [==[Carve.TsNullPacketTolerated]==] [==[Carve.TsSparseSyncNoiseRejected]==] [==[Carve.HtmlAsciiLowerUpperDoctypeAccepted]==] [==[Carve.HtmlUtf16BomChainAccepted]==] [==[Carve.HtmlNoEndTagFallsBackBounded]==] [==[Carve.HtmlNonHtmlPrefixesRejected]==] [==[Carve.SvgDeclaredSvgAccepted]==] [==[Carve.SvgBomFormAccepted]==] [==[Carve.SvgDirectKeysAccepted]==] [==[Carve.GenericXmlFallbackId]==] [==[Carve.XmlNoMarkerNoSvgRejected]==] [==[Carve.MobiBookMobiPairAccepted]==] [==[Carve.MobiOtherPalmCreatorsRejected]==] [==[Carve.MobiNoEndTagFallsBackBounded]==] [==[Carve.S007NicheBatch1SalvageSemantics]==] [==[Carve.S007NicheBatch1SharedKeyDiscriminates]==] [==[Carve.AsfTornCountFieldStillEmitted]==] [==[Carve.AsfZeroHeaderSizeEmitsThirtyByteSalvage]==] [==[Carve.AsfFilePropsDeclaredSizeDrivesExtent]==] [==[Carve.AsfIntactFileExtentUnchanged]==] [==[Carve.AsfCarveExtentCoversDataObjects]==] [==[Carve.AsfCarveStopsAtGrammarBreak]==] [==[Carve.AsfCarveTornTailSalvagesLastCompleteObject]==] [==[Carve.AsfHeaderCountGatesUnchanged]==] [==[Carve.Fh11DetectedAndWalked]==] [==[Carve.Fh11RejectsBareKeyBytes]==] [==[Carve.Fh11BoundedExtentPinned]==] [==[Carve.CptGenerationsDetected]==] [==[Carve.CptRejectsTruncatedHeader]==] [==[Carve.ZipTruncatedHeadRecovered]==] [==[Carve.ZipLfhSanityGates]==] [==[Carve.MkvDocTypeWebmAccepted]==] [==[Carve.MkvDocTypeExoticRejected]==] [==[Carve.MkvElementBoundsGate]==] [==[Carve.HeifSiblingBrandLabeledHeif]==] [==[Carve.IiuTiffEpValidatedByTiffGrammar]==] [==[Carve.IiuRw2FallbackWhenTiffEpRejects]==] [==[Carve.RafVariantHeaderAccepted]==] [==[Carve.RafRealAsciiVersionAccepted]==] [==[Carve.JpegFourthByteOutsideOgSet]==] [==[Carve.JpegTruncatedTailSalvagedAtSofEstimate]==] [==[Carve.JpegTruncatedTailWithoutSofSalvagesAtCapTenth]==] [==[Carve.JpegCompleteFileStopsAtEoiNotSofEstimate]==] [==[Carve.CabLowercaseMagic]==] [==[Carve.SpecialRejectPlainAcceptRefusedByName]==] [==[Carve.UserSignYieldsToRawSignTableAtSamePosition]==] [==[Carve.UserSignOverlapFormatsAllYieldToBuiltin]==] [==[Carve.UserSignBuiltinFloorGateSuppressesUserSignToo]==] [==[Carve.GrammarRejectedMagicNotReclaimedByUserSign]==] [==[Carve.UserSignStillFiresWhenNoBuiltinMatches]==] [==[Carve.DisabledFormatsGatesUserSignLayer]==] [==[Carve.DisabledFormatsGatesUserOnlyExtension]==] [==[Carve.DisabledOtherFormatsLeaveUserSignsOn]==] [==[Carve.CplWplTwoLayerCollisionsStayBuiltin]==] [==[Carve.Batch1OgCapsAreTheRegistryRows]==] [==[Carve.TsM2TsCapIsTheOg200MB]==] [==[Carve.TsStreamClampedAt200MB]==] [==[Carve.MidBoundTenMegabytes]==] [==[Carve.SitMinimumIsTheOgFloor]==] [==[Carve.Jp2BrandMap]==] [==[Carve.Jp2SignaturePayloadGate]==] [==[Carve.Jp2UncheckedBoxLength]==] [==[Carve.Jp2UnknownBrandRejected]==] [==[Carve.Jp2BoxWalkExtent]==] [==[Carve.Jp2EocScan]==] [==[Carve.Jp2Jp2cFinalAtSliceEdgeFallsBackBounded]==] [==[Carve.LzhMethodFamilyAccepted]==] [==[Carve.LzhEntryChainExtent]==] [==[Carve.LzhBrokenChainSalvagesLeadingEntries]==] [==[Carve.LzhLh5HeaderOnlyStridePinned]==] [==[Carve.LzhExtFlagSuppressesTheCrcStride]==] [==[Carve.LzhWrongDashByteRejects]==] [==[Carve.TextSchoolVcfAcceptedAndExtent]==] [==[Carve.TextSchoolVcfMultiCardContinuation]==] [==[Carve.TextSchoolVcfNoEndTagFallsBackBounded]==] [==[Carve.TextSchoolGpxAcceptedAndExtent]==] [==[Carve.TextSchoolGpxRootBeforeWindowRejects]==] [==[Carve.TextSchoolOspAcceptedAndRejectOnMissing]==] [==[Carve.TextSchoolPosAcceptedAndExtent]==] [==[Carve.TextSchoolChfpAcceptedAndExtent]==] [==[Carve.TextSchoolChfpRootOffAnchorRejects]==] [==[Carve.TextSchoolVcfInnerCardIsSameFile]==] [==[Carve.SvgDeclaredDocumentCarvesOnce]==] [==[Carve.GpxBomFormCarvesOnce]==] [==[Carve.DxfCanonicalRequiresFullSixteenBytePattern]==] [==[Carve.Dxf999CommentHeaderLfCarves]==] [==[Carve.Dxf999CommentHeaderCrLfCarves]==] [==[Carve.Dxf999RejectsNonCommentContinuation]==] [==[Carve.Dxf999InteriorCommentOfCanonicalFileDoesNotDoubleCarve]==] [==[Carve.OoxmlRelsPathSplit]==] [==[Carve.ZipEntriesReorderedStillTyped]==] [==[Carve.OdfEpubTypedByMimetype]==] [==[Carve.IworkAndCdrTypedByEntryName]==] [==[Carve.OfficeUnrefinableStaysDocxAndPlainZipStaysZip]==] [==[Carve.ZipTypingXmindContentBranch]==] [==[Carve.ZipTypingXmindNeedleMissStaysGeneric]==] [==[Carve.ZipTypingDocumentToolEntryNames]==] [==[Carve.ZipTypingLaterEntryOverwritesEarlierStamp]==] [==[Carve.ZipTypingUnrelatedEntriesStayGeneric]==] [==[Carve.ZipTypedIdDisableGateSuppressesTyping]==] [==[Carve.MultiEntryZipCarvesOnce]==] [==[Carve.DbxPlainMagicCarvesWithDeclaredSize]==] [==[Carve.DbxAllFourGuidsCarve]==] [==[Carve.DbxJmf6VariantCarves]==] [==[Carve.DbxJmf6WrongVersionWordRejected]==] [==[Carve.DbxMarkedFormZeroSectorWalk]==] [==[Carve.DbxMarkedFormOddPrefixRoundsEven]==] [==[Carve.DbxRejectsWrongGuid]==] [==[Carve.DbxRejectsTruncatedMarkedPattern]==] [==[Carve.DbxRegistryCarriesAllThreeKeys]==] [==[Carve.CslSizeFieldExtendsPastCap]==] [==[Carve.CslCd01OutOfCapDeclinedIsDropped]==] [==[Carve.CslShortCd01DeclinedIsDropped]==] [==[Carve.SvgLegUnaffectedByCslBranch]==] [==[Carve.HikPackSystemHeaderPrivateStreamAccepted]==] [==[Carve.HikDirectPrivateStreamAccepted]==] [==[Carve.HikGenericPsNotShadowed]==] [==[Carve.HikOneGiBCapIsTheRowCap]==] [==[Carve.HikDiscriminatorIsPrivateStream2]==] [==[Carve.HikDirectPathAcceptsBcOnly]==] [==[Carve.MpegPsPesLengthsAreBigEndian]==] [==[Carve.OomWindowBufferReturnsCode]==] [==[Carve.OomTailBufferReturnsCode]==] [==[Carve.OomNoExceptionEscapesEngine]==] [==[Carve.AepRifxAccepted]==] [==[Carve.AepXmpEndExtent]==] [==[Carve.AepBeLengthByteSwap]==] [==[Carve.AepCapThirtyMiB]==] [==[Carve.OneChunkChainWalksPastTheStandInCap]==] [==[Carve.OneFirstChunkZeroSizePublishesTwoDeviceFlat]==] [==[Carve.OneMidChainOversizeChunkPublishesTwoDeviceFlat]==] [==[Carve.OneChunkChainHostileVariantsSalvageAndRefuse]==] [==[Carve.ThreeDsSectionWalkUsesTheStrideFields]==] [==[Carve.X3fSectionDirectoryWalkCarvesTrueEnd]==] [==[Carve.X3fTornSalvagesTheWalkedPrefix]==] [==[RawSigns.UnitsAreKib]==] [==[RawSigns.CarvesFiveMiBVob]==] [==[RawSigns.MinFloorOneKib]==] [==[RawSigns.TailPairWindowScaled]==] [==[UserSigns.ParseTheOgsShape]==] [==[UserSigns.ParseOgShippedExtremes]==] [==[UserSigns.ManifestEightCharExtParses]==] [==[UserSigns.HostileBlobStillAtomic]==] [==[UserSigns.RegistrationRoundTripProceedsToScan]==] [==[UserSigns.ShortFragmentMatchesInCore]==] [==[UserSigns.DefaultSetShipsAndLoads]==] [==[UserSigns.OgAbsentFromStockExtensionsCarried]==] [==[UserSigns.LowercaseHexSignLiveAndRegistered]==] [==[UserSigns.UppercaseHexSignMatches]==] [==[UserSigns.DefaultSetEffectiveCount]==] [==[UserSigns.UnknownMemberIgnored]==] [==[UserSigns.MemberNamesCaseInsensitive]==] [==[UserSigns.MalformedSignSkippedNotFatal]==] [==[UserSigns.OddHexSkipped]==] [==[UserSigns.AllMalformedSignsLoadStructurally]==] [==[UserSigns.TornFileStillRejected]==] [==[UserSigns.ShippedOgFileVerbatimLoads]==] [==[UserSigns.DuplicateKeySemantics]==] [==[UserSigns.ShippedCensusDedupTruth]==] [==[CarveStats.UserSignFilesCountInCarvedFound]==] [==[CarveStats.StockRawSignFilesCountInCarvedFound]==] [==[CarveStats.UserSignCarvedFoundSessionRoundTrip]==] [==[ConfigDisplay.SummaryFamilyUnificationGroups]==] [==[ConfigDisplay.SummaryFamilyUngroupedStandsAlone]==] [==[ConfigDisplay.SummaryFamilyNullAndEmptySafe]==] [==[ConfigDisplay.TypeDisplayOrderVerbatim]==] [==[ConfigDisplay.TypeDisplayOrderUnrankedReturnsFalse]==] [==[ConfigDisplay.TypeDisplayCategoryNamesVerbatim]==] [==[ProgressDisplay.RemapCurveTable]==] [==[ProgressDisplay.StallCapIsTheDefensiveBranch]==] [==[ProgressDisplay.PercentTypeDispatch]==] [==[ProgressDisplay.MonotonicPublishGuard]==] [==[ProgressDisplay.CorePermilleStaysLinear]==] [==[Eta.WholeRunModel]==] [==[Eta.EmaDamping]==] [==[Eta.TimeBudgetVariant]==] [==[Eta.PlaceholderAndCountdownFormat]==] [==[Eta.ParameterErrorClass]==] [==[VolumeLabel.Ext4FromSuperblock]==] [==[VolumeLabel.Ext4LabellessYieldsEmpty]==] [==[VolumeLabel.FatFromBootSector]==] [==[VolumeLabel.FatLabellessYieldsEmpty]==] [==[VolumeLabel.NtfsFromVolumeRecord]==] [==[VolumeLabel.CallerValueWins]==] [==[VolumeLabel.UnknownFamilyLeavesFallback]==] [==[Crsf.RoundTrip]==] [==[Crsf.ExtentLogicalRoundTrip]==] [==[Crsf.CorruptionSkipsForward]==] [==[Crsf.TreeRoundTrip]==] [==[Crsf.TinyChunkDeflatesWhenSmaller]==] [==[Crsf.ReconstructedCarveTreeRoundTrip]==] [==[Crsf.VolumeInfoIdentityCardRoundTrip]==] [==[Crsf.TornCheckpointSalvagesPrefix]==] [==[Crsf.PeekRecordHeaderOnly]==] [==[Crsf.LostDirFoldersRoundTrip]==] [==[SessionTree.RefreshStatsIsRecursive]==] [==[SessionTree.ReloadedRootStatsAreRecursive]==] [==[SessionTree.StatsDeletedCountRecursive]==] [==[Session.GradeRoleAndProvenanceRoundTrip]==] [==[Session.OldFormatStillLoads]==] [==[Session.ForwardParentDeferredAndCounted]==] [==[Session.BadMarkerFailsTheRecord]==] [==[Session.TruncatedTailFailsAfterFullCount]==] [==[Session.HostileNameAndRoleLengthBombs]==] [==[Session.CorruptRecordEarlyExitsReleaseProps]==] [==[Session.RawNameRoundTrip]==] [==[Session.OldFormatWithoutRawNameStillLoads]==] [==[Session.SaveEmitsProgress]==] [==[Session.SaveCancelsCleanly]==] [==[Session.AutoNamingConvention]==] [==[Session.BtrfsExtrasRoundTrip]==] [==[Session.ZfsSecondaryFlagsRoundTrip]==] [==[Session.EngineExtrasAbsentWhenUnpopulated]==] [==[Session.FatOriginClusterRoundTrip]==] [==[Session.NtfsOriginLcnRoundTrip]==] [==[Session.NtfsTotalClustersRoundTrip]==] [==[Session.NtfsParentRefRoundTrip]==] [==[Session.FatParentClusterRoundTrip]==] [==[Session.IdentityCardFirmwareWwnInterface]==] [==[Session.BootSnapshotRoundTrip]==] [==[Session.SaveUnixVTailRoundTrip]==] [==[Session.DeserializeProgressPerNodeAndCancel]==] [==[Session.LoadSessionSummaryPropagatesProgressAndCancel]==] [==[Session.SizeExceedsBlocksFlagRoundTrip]==] [==[SessionSummary.ReadSummaryLightRecordsOnly]==] [==[SessionSummary.LegacySessionWithoutVolumeInfo]==] [==[Orchestrator.DynamicDiskVolumeScan]==] [==[Orchestrator.WholeDiskScansTableVolumes]==] [==[Orchestrator.WholeDiskGptTwin]==] [==[Orchestrator.WholeDiskMixedFamilies]==] [==[Orchestrator.WholeDiskFsMarkerNamesForeignVolume]==] [==[Orchestrator.WholeDiskDamagedVolumeSkippedHonestly]==] [==[Orchestrator.WholeDiskSuperfloppyUnchanged]==] [==[Orchestrator.WholeDiskNoDoubleScan]==] [==[Orchestrator.WholeDiskVolumeRootsCarryDisplayNames]==] [==[Orchestrator.WholeDiskKnownVsLostDistinguishable]==] [==[Orchestrator.LostPartUnnamedCandidateNamedLostPartitionN]==] [==[Orchestrator.LostPartNamedCandidateKeepsVolumeLabel]==] [==[Orchestrator.LostPartInsertionCapTenTargets]==] [==[Orchestrator.LostPartCandidateListingCarriesLayoutFields]==] [==[Orchestrator.WholeDiskVolumeRootNamesRoundTripSession]==] [==[Orchestrator.JunkFilteredFromResults]==] [==[Orchestrator.LogSilentByDefault]==] [==[Orchestrator.LogSinkReceivesScanBrackets]==] [==[Orchestrator.MemoryMonitorEmitsUnderPressure]==] [==[Orchestrator.WeightedProgress]==] [==[Orchestrator.ProbeFamily]==] [==[Orchestrator.SpoofedExfatOemRejectedBeforeEngine]==] [==[Orchestrator.SpoofedFatBootUnreadableRootRejected]==] [==[Orchestrator.RealExfatBootContentVerified]==] [==[Orchestrator.FsFamilyValuesMatchContract]==] [==[Orchestrator.UdfProbeFailsOnMkudffsShapeLikeTheOg]==] [==[Orchestrator.ProbesUdfVolume]==] [==[Orchestrator.UdfProbeRequiresBothDescriptorReads]==] [==[Orchestrator.UdfNotConfusedWithFat]==] [==[Orchestrator.UdfSurfacesAsCarveFamily]==] [==[Orchestrator.CarveSingleNamingFamily]==] [==[Orchestrator.CarveTallySessionRoundTrip]==] [==[Orchestrator.CarveVolumeGroupTreeShape]==] [==[Orchestrator.CarveVolumeGroupLazyNoCarvesNoChild]==] [==[Orchestrator.CarveVolumeGroupNameForms]==] [==[Orchestrator.StagedRunAndSessionRoundTrip]==] [==[Orchestrator.PreScanRepairRestoresTornFatAndScans]==] [==[Orchestrator.PreScanRepairDeniedNeverWrites]==] [==[Orchestrator.PreScanRepairWithoutHooksNeverWrites]==] [==[Orchestrator.ReadPipeBuildsCacheOverRetryForValidGeometry]==] [==[Orchestrator.ReadPipeDegradesToBareSliceWhenCacheCreationFails]==] [==[Orchestrator.ReadPipeZeroCapacityDegradesToBareSlice]==] [==[Orchestrator.LogSinkRegistrySurvivesHeavyChurn]==] [==[Orchestrator.IdentityCardV4SnapshotAndAxes]==] [==[Orchestrator.DynamicDiskScanStampsBasisAndSessionRoundTrips]==] [==[Orchestrator.LostRootNamedFilePathLost]==] [==[Orchestrator.TelemetryCountersMoveDuringCarveStage]==] [==[Orchestrator.SpannedLdmTransientMemberFaultScans]==] [==[Orchestrator.LdmMemberResolversConstructThroughBuildReadPipe]==] [==[FilterFolder.CorpusListsPinned]==] [==[FilterFolder.JunkGateFixtureFileScansAndFlagsHiddenSet]==] [==[SafeWriter.OverlapRefusal]==] [==[SafeWriter.ExplicitSelectionStillRecoversJunkFlaggedNode]==] [==[SafeWriter.CollisionAndSanitize]==] [==[SafeWriter.ResidentPrefixNotOverwritten]==] [==[SafeWriter.InvalidUtf8NameRestoredFromRaw]==] [==[ScanEvents.ErrorRangeGoesToLiveSink]==] [==[ScanEvents.SinkSwitchDeliversAllTypes]==] [==[ScanEvents.SingleBoundaryAtTeardown]==] [==[ScanEvents.ErrorRangeCarriesModuleOperationAttempts]==] [==[ScanEvents.ProbeBlockFailureAttributesToOrchestrator]==] [==[ScanEvents.CarveErrorRangeAttributesToCarveModule]==] [==[ScanConfig.DeepStageDefaultsOnAndWholeDiskOff]==] [==[Transport.PeGateDecisionMatchesOgProbe]==] [==[Transport.QuiesceChainConstantsMatchCorpus]==] [==[Transport.EnumerateBoundMatchesDriverGrammarSpace]==] [==[Transport.DisconnectClassifiedDistinctly]==] [==[Transport.MediaErrorsStillReadFailed]==] [==[DriverStatus.ReadyWhenAllFacetsGreen]==] [==[DriverStatus.AbsentWhenServiceNotInstalled]==] [==[DriverStatus.NotStartedWhenInstalledButStopped]==] [==[DriverStatus.UnauthorizedWhenOpenDeniedByDacl]==] [==[DriverStatus.ErroredWhenOpenFailsWithAnyOtherGle]==] [==[DriverStatus.ErroredWhenGetInfoFailsOnOpenedDevice]==] [==[DriverStatus.VersionMismatchWhenMajorDiffers]==] [==[DriverStatus.MinorOnlyDivergenceStaysReady]==] [==[DriverStatus.ClassificationOrderIsFailFastFirstMatch]==] [==[DriverStatus.StateNamesAndErrorCodesAreTotalFunctions]==] [==[DriverStatus.DefaultStatusIsAbsentNotReady]==] [==[DriverStatus.RequiredWireMajorMatchesDriverContract]==] [==[ScanTruth.FullyUnreadableVolumeRefusesInsteadOfFakeCompleting]==] [==[ScanTruth.PartialDamageKeepsSalvageSemanticsAndSurfacesCoverage]==] [==[ScanTruth.BlindProbeIsFlaggedWhileTheRestReads]==] [==[ScanTruth.HealthyZeroVolumeStillCompletesCleanly]==] [==[ScanTruth.StatsFieldsAreZeroInitialized]==] [==[PauseResume.TokenCheckpointBlocksAndResumes]==] [==[PauseResume.CancelWhilePausedWakesWaiter]==] [==[PauseResume.PausedMsAccounting]==] [==[PauseResume.ScanPauseFreezesAndElapsedExcludesPause]==] [==[PauseResume.SaveOnCancelStampsTruthfulMaskAndResumeSkipsQuick]==] [==[PauseResume.MaskIntegrityViolationRefusesResume]==] [==[PauseResume.ResumedSessionPreservesOriginalConfiguredMask]==] [==[PauseResume.SessionRestoresVolumeInfoAndCandidates]==] [==[PauseResume.SaveAfterResumeReloadsCleanly]==] [==[ScanStats.BytesFoundAggregates]==] [==[ScanStats.BytesFoundSessionRoundTrip]==] [==[Checkpoint.StageBoundaryCheckpointResumesAfterCrash]==] [==[Checkpoint.CorruptMagicRefused]==] [==[Checkpoint.CommitHookFiresAfterEveryFinalizeAndAbortsOnError]==] [==[ExtentBasis.TreeRebaseShiftsOnceAndIsIdempotent]==] [==[ExtentBasis.SliceScanSavesAbsoluteExtents]==] [==[ExtentBasis.LegacySliceRelativeSessionNormalizedOnLoad]==] [==[Telemetry.UniformProgressAcrossStagesMonotonic]==] [==[Telemetry.FatWalkEmitsIncrementalTicks]==] [==[Telemetry.PerStageShapeAndCompletion]==] [==[Telemetry.LiveErrorMapVisibleMidScan]==] [==[Telemetry.QueryDuringScanIsThreadSafe]==] [==[Telemetry.LiveAttachCounterTracksTheTree]==] [==[Telemetry.CountersMoveDuringStage]==] [==[WriterDirs.RestoresTimestampsAndAttributes]==] [==[WriterDirs.ZeroTimeSlotsPassAsUnset]==] [==[WriterDirs.MetadataFailureIsNonFatal]==] [==[WriterDirs.MetadatalessBackendReportsUnsupported]==] [==[WriterDirs.InteriorHoleZeroFilledFullSize]==] [==[WriterDirs.LegacyCumulativeExtentsUnchanged]==] [==[WriterDirs.StructureReconstructionWithEmptyLeaves]==] [==[WriterDirs.DeepDestChainCreatedAndIdempotentRerun]==] [==[WriterDirs.IntraQueueDuplicatesRenameUnderSkip]==] [==[WriterDirs.PerItemErrorCollectedQueueContinues]==] [==[WriterDirs.MetadataOnlyItemSkipsNotZeroFills]==] [==[WriterDirs.ProgressExcludesErrorItems]==] [==[WriterDirs.CollisionNameUnderscoreFormat]==] [==[WriterDirs.CollisionExhaustionFailsTheItem]==] [==[WriterDirs.MkdirSemanticsMatchShCreateDirectoryEx]==] [==[WriterReadPipe.TransientFailureRetriedNotZeroFilled]==] [==[WriterReadPipe.HardFailureZeroFillsOnlyAfterRetriesSpent]==] [==[WriterReadPipe.BareDeviceLegacyShapeZeroFillsTransients]==] [==[WriterReadPipe.ConstructionIsTheScanSubstrate]==] [==[WriterReadPipe.ReadErrorMarksTheLiveNode]==] [==[WriterReadPipe.SettingsGateSkipsReadErrorAccounting]==] [==[LdmRecovery.SpanRecoveryReadsThroughVirtualBasis]==] [==[LdmRecovery.StripeRecoveryRoundTrips]==] [==[LdmRecovery.Raid5RecoveryWithMissingMember]==] [==[LdmRecovery.OverlapRefusalMapsToPhysicalSpans]==] [==[LdmRecovery.SimpleVolumeRecoveryRoundTrip]==] [==[LdmRecovery.PreviewMatchesRecovery]==] [==[LdmRecovery.SessionRoundTripsLayoutsAndRootBasis]==] [==[Writer.ReadsRetryOnTransientFailure]==] [==[Writer.BareDeviceTruncatesWhatThePipeRetries]==] [==[Writer.RecoveryCacheServesSharedExtents]==] [==[Writer.RecoveryEntryPointsConstructThroughBuildReadPipe]==] [==[Writer.HardReadFailureTruncatesAndKeeps]==] [==[Writer.ReadErrorLogTriFormat]==] [==[Writer.FailureCauseTableMatchesOgRuntimeStrings]==] [==[Writer.BeyondVolumeReadRecordsCauseFourAndClampsStraddle]==] [==[Writer.ShortExtentMetadataCommitsPartial]==] [==[Writer.CompressedUndecodableBlockStillSalvages]==] [==[Reghive.ParsesValidHiveAndRejectsGarbage]==] [==[Reghive.RejectsCorruptedHeaderChecksum]==] [==[Reghive.WalksControlSetKeyFamilyCaseInsensitive]==] [==[Reghive.EnumeratesSubkeyNames]==] [==[Reghive.ReadsBinaryValueDataPointedAndInline]==] [==[Reghive.UnicodeKeyNamesWalkAsAscii]==] [==[Reghive.TruncatedAndMutatedHivesParseOrRefuseWithoutCrashing]==] [==[FveAutoUnlock.HarvestsPerGuidDataBlobs]==] [==[FveAutoUnlock.HarvestCapsBlobsAt256Bytes]==] [==[FveAutoUnlock.HarvestCapsRecordsAt100AndSkipsTinyBlobs]==] [==[FveAutoUnlock.SelectionPrefersMatchingVolumeGuid]==] [==[OfflineOs.ProfileReadsCurrentVersion]==] [==[OfflineOs.ControlSetResolvesViaSelectWithFallback]==] [==[OfflineOs.SystemPartitionFromMountedDevices]==] [==[OfflineOs.ServerVsClientClassification]==] [==[OfflineOs.ToleratesMissingKeysAndOddValueShapes]==] [==[BcdFix.BcdFindsBootPartitionBySignature]==] [==[BcdFix.BcdOffsetRewriteWritesNewLba]==] [==[BcdFix.BcdUndoRestoresOffsets]==] [==[BcdFix.BootfixRefusedWithoutConsent]==] [==[BcdFix.BoundariesNeverPlan]==] [==[NvramFix.MatchesMbrBySignatureStartSize]==] [==[NvramFix.MatchesGptByGuidAlone]==] [==[NvramFix.MbrFlagsMayRideDifferentNodes]==] [==[NvramFix.PatchUpdatesLoadOption]==] [==[NvramFix.UnmatchedEntriesUntouched]==] [==[NvramFix.ConsentDefaultDeny]==] [==[NvramFix.UndoRestoresOriginalVariables]==] [==[NvramFix.SetFailureStopsAndReports]==] [==[NvramFix.MalformedDevicePathBounded]==] [==[NvramFix.NullSeamRejected]==] [==[NvramFix.AbsentBootOrderIsSilentNoop]==] [==[NvramFix.MissingBootVariableStopsTheWalk]==] [==[NvramFix.EmptyPlanIsNoOp]==] [==[Preview.ExtentAccurateReadsWithSparseTailAndEof]==] [==[Preview.BoundedReadCannotBeTricked]==] [==[Preview.ReadFailuresZeroFilledAndReported]==] [==[Preview.CompressedExtentDecodes]==] [==[Preview.ConcurrentIndependentReaders]==] [==[Preview.MalformedCapsExtentKeepsAlignment]==] [==[Preview.M077SizeGate100MiB]==] [==[Preview.M077TimeGuardDefaultsOnAndOverridable]==] [==[PreviewMeta.JpegDimensionsAndExif]==] [==[PreviewMeta.PngGifBmpDimensions]==] [==[PreviewMeta.WavDurationAndMp4Brand]==] [==[PreviewMeta.AviMetadataExtracted]==] [==[PreviewMeta.MkvMetadataExtracted]==] [==[PreviewMeta.FlacMetadataExtracted]==] [==[PreviewMeta.Mp3DurationEstimated]==] [==[PreviewMeta.M4aTypedAsMp4Family]==] [==[PreviewMeta.PdfTextAndPlainAndUnsupported]==] [==[PreviewMeta.TruncatedMagicsAreSafeAndHonest]==] [==[PreviewMeta.WavFmtChunkOutsideHeadCache]==] [==[PreviewMeta.DocxRawDeflateInflatesAndPlainZipStaysZip]==] [==[PreviewMeta.DocxCapsMaxReadHonored]==] [==[PreviewMeta.DocxTailBeyondBudgetDegradesToZip]==] [==[PreviewMeta.CompressedExtentReportsTruncatedUnderBudget]==] [==[PreviewMeta.CompressedExtentWholeBlockReadDisclosed]==] [==[PreviewMeta.ArchiveEnumeratesEntries]==] [==[PreviewMeta.ArchiveDamagedLocalHeaderStillEnumerates]==] [==[PreviewMeta.ArchiveDocxUpgradeCoexistsWithEnumeration]==] [==[PreviewMeta.ArchiveEntryCapAndTruncatedFlag]==] [==[PreviewMeta.MetadataTruncatedFlagTruthfulAllPaths]==] [==[PreviewMeta.RandomSmallInputsAreSafe]==] [==[PreviewMeta.Utf8BomTextIsText]==] [==[PreviewMeta.Utf16BomTextIsText]==] [==[PreviewMeta.Utf16SurrogatePairDecodes]==] [==[PreviewMeta.Latin1TextIsText]==] [==[PreviewMeta.PrintableHeadBinaryNotText]==] [==[PreviewMeta.ExtensionlessNameIsNotText]==] [==[PreviewMeta.WhitelistMembershipCaseInsensitive]==] [==[PreviewDocs.NoIInWindowStreamDriftAndNoMovGhost]==] [==[PreviewTextWhitelist.ByteExactAgainstCorpus]==] [==[Thumbnail.StatusNamesAreStable]==] [==[Thumbnail.TextSnapshotRenders]==] [==[Thumbnail.TextSnapshotHundredByteBound]==] [==[Thumbnail.TextSnapshotControlsAreBlankAndEmptyRefuses]==] [==[Thumbnail.TextSnapshotUtf16BomDecodes]==] [==[Thumbnail.FormatSupportIsTheDocumentedV1Matrix]==] [==[Thumbnail.MetafileAndIconFamiliesAreNamedRefusals]==] [==[Thumbnail.CapsDefaultsPinTheCorpusAnchors]==] [==[Thumbnail.PngDecodesAndDownscalesExactly]==] [==[Thumbnail.OrdinaryPhotoDefaultBudget]==] [==[Thumbnail.EmbeddedFastPathOnByteOversize]==] [==[Thumbnail.RawTiffFamilyServesJpegPreview]==] [==[Thumbnail.JpegDecodesWithLossyTolerance]==] [==[Thumbnail.BmpDecodesExactly]==] [==[Thumbnail.GifDecodesFirstFrameExactly]==] [==[Thumbnail.AspectPreservedBothAxesNeverUpscaled]==] [==[Thumbnail.SmallImageIsNotUpscaled]==] [==[Thumbnail.BombGuardRefusesBeforeDecode]==] [==[Thumbnail.OversizedJpegUsesExifEmbeddedThumbnail]==] [==[Thumbnail.SmallJpegWithEmbeddedThumbPrefersFullDecode]==] [==[Thumbnail.TiffOrientationApplied]==] [==[Thumbnail.TiffOrientationInvalidIgnored]==] [==[Thumbnail.EmbeddedFastPathServesStoredOrientationByDefault]==] [==[Thumbnail.EmbeddedFastPathHonorsIfd1DeclaredOrientation]==] [==[Thumbnail.TruncatedPngFailsHonestly]==] [==[Thumbnail.TruncatedJpegIsDisclosedNotSilent]==] [==[Thumbnail.BudgetExceededIsReported]==] [==[Thumbnail.UnsupportedFormatsAreNamed]==] [==[Thumbnail.UnreadableSpanIsNeverThumbnailed]==] [==[Thumbnail.BadParametersAreRejected]==] [==[Thumbnail.EmptyInputIsUnsupported]==] [==[Thumbnail.ExtentBackedDecodesLikeResident]==] [==[Thumbnail.LargeStillImageWithinOgBand]==] [==[Thumbnail.OversizePastOgGateRefused]==] [==[Snapshot.CaptureFidelityAndDfsParity]==] [==[Snapshot.PartialFlagAndEmptyTree]==] [==[Snapshot.ConcurrentEmitCaptureWalk]==] [==[Snapshot.RecoveryFromSnapshotIsByteIdentical]==] [==[Query.CategoryMap]==] [==[Query.EmailCategoryExists]==] [==[Query.BookmarksCategory]==] [==[Query.CameraRawClassified]==] [==[Query.TableCountMatchesCorpusUniques]==] [==[Query.CrossListedFirstWriterWins]==] [==[Query.FilterSortPageCorrectness]==] [==[Query.CarvedVolumeFilterAxis]==] [==[Query.PathPrefixFilterMatchesAndPrunes]==] [==[Query.M072PathSubstringAndStateMask]==] [==[Query.RandomizedAgainstReference]==] [==[Query.TypeSortKeyOrdersByCategoryThenExtension]==] [==[Query.ModifiedUnitIsFiletimeEverywhere]==] [==[Query.DateBucketsTranslateToRanges]==] [==[Query.SizeBucketsTranslateToRanges]==] [==[Query.BucketFiltersThroughRun]==] [==[Query.UnsavedFileCategory]==] [==[Query.CategoriesAppendOnlyGuard]==] [==[Error.ErrTextCoversEveryCode]==] [==[Display.SizeLogFormatterBoundaries]==] [==[Display.SizeUiFormatterTiers]==] [==[Display.SizeUiFormatterPrecisionAndStrip]==] [==[FsCheck.FatCopyDisagreementReported]==] [==[FsCheck.FatDuplicateRefDetected]==] [==[FsCheck.FatEntryHoleReported]==] [==[FsCheck.FatCrossLinkedChainReported]==] [==[FsCheck.DirDotPairMissingRejected]==] [==[FsCheck.DirCycleDetected]==] [==[FsCheck.LiveFileSizeNeChain]==] [==[FsCheck.OrphanClustersReported]==] [==[FsCheck.CleanFatVolumeReportsNoErrors]==] [==[FsCheck.DbrCeilingRejectsOversizedClusterCount]==] [==[FsCheck.NtfsCleanVolumeReportsNoErrors]==] [==[FsCheck.NtfsVersionGate]==] [==[FsCheck.MftRecordNotNamedMft]==] [==[FsCheck.MftBitmapCrossLink]==] [==[FsCheck.BitmapFreeButAllocated]==] [==[FsCheck.CheckNeverAbortsScan]==] [==[FsCheck.ReportSessionRoundTrip]==] [==[QueryPerf.MillionNodesFilterCountUnder200ms]==] [==[DiskMap.RangeFreeEdgeMatrix]==] [==[DiskMap.FreeMapRecordRoundTripAndRejection]==] [==[DiskMap.FreeOverlayHonorsZoomWindow]==] [==[DiskMap.DownsamplePreservesRawTruth]==] [==[DiskMap.FreeNeverMarksFoundContentCells]==] [==[DiskMap.OverlayBudgetGrowsCellSize]==] [==[DiskMap.FreeNeverOverridesUsedOrBadAndDownsampleIsConservative]==] [==[Apply.PlansEntryIntoFirstFreeSlot]==] [==[Apply.ExecuteWritesEntryVerifiesAndUndoes]==] [==[Apply.RefusedWithoutConsent]==] [==[Apply.StaleTableRefusedBeforeConsent]==] [==[Apply.QuiesceFailureIsACleanRefusal]==] [==[Apply.WriteFailureRollsBackEverything]==] [==[Apply.ContainmentRefusals]==] [==[Apply.SignatureRestorePlannedWhenAbsent]==] [==[Apply.ChsClampsBeyondCylinder1023]==] [==[Apply.FamilyTypeMap]==] [==[LostApply.WritesEntryAndRefreshesModernPath]==] [==[LostApply.LegacyPathMatchesCorpusSequence]==] [==[LostApply.GptRebuildRegeneratesSignature]==] [==[LostApply.ConsentBeforeLockDenialHoldsNothing]==] [==[LostApply.UndoApplyRestoresAndRefreshes]==] [==[LostApply.RefreshFailureIsReportedNotSwallowed]==] [==[LostApply.RollsBackWhenVerifyFails]==] [==[LostApply.PlanIsReadOnlyAndRefusalsAreResults]==] [==[RawFixer.CleanExfatNeverTouched]==] [==[RawFixer.ExfatChecksumRepair]==] [==[RawFixer.ExfatDestroyedPrimaryRestoredFromBackup]==] [==[RawFixer.NtfsBackupRestore]==] [==[RawFixer.NtfsCleanRefused]==] [==[RawFixer.NtfsNoBackupRefused]==] [==[RawFixer.Fat32BackupRestore]==] [==[RawFixer.ExfatGeometryRederivation]==] [==[RawFixer.ExfatAmbiguousGeometryRefused]==] [==[RawFixer.UnknownFamilyRefused]==] [==[RawFixer.ExecuteRefusesEmptyPlan]==] [==[RawFixer.FailedWriteRollsBackEverything]==] [==[RawFixer.UndoLogRoundTrip]==] [==[RawFixer.MftRecord0RestoredFromMirror]==] [==[RawFixer.MftCleanNeverTouched]==] [==[RawFixer.MftBothInvalidNoWrite]==] [==[RawFixer.MftRestoreMakesVolumeOpenable]==] [==[RawFixer.FatTornMirrorCopyRestoredSectorGranular]==] [==[RawFixer.FatTornPrimaryRestoredFromMirror]==] [==[RawFixer.FatMirrorAmbiguityNeverWritten]==] [==[RawFixer.FatCleanMirrorNoAction]==] [==[RawFixer.FixByScanSectorRebuildsBootFromOnlyCandidate]==] [==[RawFixer.FixByScanSectorAmbiguousCandidatesRefused]==] [==[RawFixer.FixByScanSectorIdenticalCandidatesNotAmbiguous]==] [==[RawFixer.RebasedPartitionPlanWritesAtDiskBasisNeverLba0]==] [==[RawFixer.VolumeLockAcquiredBeforeFirstWrite]==] [==[RawFixer.VolumeLockFailureNeverWrites]==] [==[RawFixer.UnlockHappensOnRollbackPath]==] [==[RawFixer.NtfsPrimaryDamaged4Kn]==] [==[RawFixer.ExfatViaBackup4Kn]==] [==[RawFixer.AnalyzeAllBranches4KnAsan]==] [==[RawFixer.ExfatRestorePlanIsSectorSized4Kn]==] [==[RawFixer.ExfatBackupReadFull4Kn]==] [==[Consistency.ExtensionIsLastDotUppercased]==] [==[Consistency.CheckableVocabularyIsSignatureFamiliesPlusSpecials]==] [==[Consistency.ClassifierIsSubstringPlusVideoSpecials]==] [==[Lost.ContentInconsistentGhostRejected]==] [==[Lost.ContentConsistentGhostAccepted]==] [==[Lost.RatioBoundaryThirtyPercent]==] [==[Lost.MajorityGateRequiresHalfFilterable]==] [==[Lost.DirectoriesAndExtentlessFilesMirrorOgCounters]==] [==[Lost.DataStartOutOfBoundsInvalidatesTheCandidate]==] [==[Lost.UnreadableDataStartStaysUnmatched]==] [==[Lost.EmptyCandidateListNeverRejects]==] [==[Lost.WholeDiskScanDropsContentInconsistentExfatGhost]==] [==[Lost.WholeDiskScanKeepsContentConsistentExfatGhost]==] [==[TreeDetach.RemovesNodesRootsAndIndexes]==] [==[TreeDetach.HardLinkPropsSurviveWithTheirRefs]==] [==[Refs.BootValidation]==] [==[Refs.FullScanHierarchy]==] [==[Refs.FallbackRootSkipsOgSystemFamilies]==] [==[Refs.DeletedGradingAndId2]==] [==[Refs.OrchestratorProbeAndScan]==] [==[Refs.RecognitionRequiresFsrsMarker]==] [==[Refs.NextChainFindsNewerTable]==] [==[Refs.CheckpointLayoutMinor14]==] [==[Refs.Pre34CheckpointRejected]==] [==[Refs.CheckpointBodyMinorGatesLayoutNotVbr]==] [==[Refs.VbrMinor14BodyMinorOldListStillOldLayout]==] [==[Refs.ResidentInlineFileRecovered]==] [==[Refs.ResidentDollarIMetadataRead]==] [==[Refs.MultipleDataAttributesMerged]==] [==[Refs.PropertyFlagAndUnknownRowsDoNotBreakDataWalk]==] [==[Refs.PagedRunTableFollowed]==] [==[Refs.Gen7OldGrammarDatarunAttribute]==] [==[Refs.RunlistGranularityMismatchDiscarded]==] [==[Refs.StaleMsbPageDeletedFileRecovered]==] [==[Refs.StaleSweepSkipsLivePages]==] [==[Refs.HardLinkRowsCollapseToOneResult]==] [==[Refs.RecycleRoutedFlaggedAndDollarIRemap]==] [==[Refs.ProgressMonotonicIntermediateTicks]==] [==[Refs.ProgressCancellationMidWalk]==] [==[Refs.Bps256Accepted]==] [==[Refs.Bps1024Accepted]==] [==[Refs.Cluster16KRejected]==] [==[Refs.Cluster64KAccepted]==] [==[Refs.OpenReadIsFourKiB]==] [==[Refs.ContainerRelativeRunTranslatedPerOgFormula]==] [==[Refs.PhysicalRunFlag20Untranslated]==] [==[Refs.ContainerBlockBelowSpanStaysPhysical]==] [==[Refs.SingleContainerExtentsUnchanged]==] [==[Refs.UnnamedDataOnlyRowEmitted]==] [==[Refs.UnnamedRowGradeIsContentBased]==] [==[Refs.UnnamedOddLengthKeyEmittedNoCrash]==] [==[Refs.ZeroSizeNoDotRowSuppressed]==] [==[Refs.ZeroSizeWithExtensionEmitted]==] [==[Refs.NonZeroNoDotEmitted]==] [==[Refs.UnnamedZeroSizeRowSuppressed]==] [==[Refs.UnnamedNameSessionRoundTrip]==] [==[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/1]==] [==[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/2]==] [==[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/3]==] [==[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/4]==] [==[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/5]==] [==[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/6]==] [==[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/7]==] [==[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/8]==] [==[ExifOrientation/ExifOrientationTest.AppliesOgRotateFlip/9]==] [==[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_rgb_le.tif"]==] [==[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_rgb_be.tif"]==] [==[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_packbits.tif"]==] [==[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_deflate.tif"]==] [==[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_palette.tif"]==] [==[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_gray16.tif"]==] [==[TiffVariants/ThumbnailTiff.BaselineVariantsDecodeExactly/"tiff_lzw.tif"]==])
