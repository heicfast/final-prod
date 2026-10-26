add_test([=[Fixup.ValidRecordApplies]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Fixup.ValidRecordApplies]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Fixup.ValidRecordApplies]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fixup.cpp:42]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Fixup.TornRecordRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Fixup.TornRecordRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Fixup.TornRecordRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fixup.cpp:53]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Fixup.TornThirdTailStillSalvaged]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Fixup.TornThirdTailStillSalvaged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Fixup.TornThirdTailStillSalvaged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fixup.cpp:67]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Fixup.MinSizeGate]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Fixup.MinSizeGate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Fixup.MinSizeGate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fixup.cpp:86]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Fixup.ZeroOffsetRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Fixup.ZeroOffsetRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Fixup.ZeroOffsetRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fixup.cpp:93]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Fixup.FourKnSectorSize]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Fixup.FourKnSectorSize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Fixup.FourKnSectorSize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fixup.cpp:102]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.FlatFileScansMbr]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.FlatFileScansMbr]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.FlatFileScansMbr]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:122]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.FlatRejectsEmptyAndMissing]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.FlatRejectsEmptyAndMissing]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.FlatRejectsEmptyAndMissing]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:154]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.ImageRejectsTruncatedFile]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.ImageRejectsTruncatedFile]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.ImageRejectsTruncatedFile]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:164]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.EdiskRefusedAsRegisteredExclusion]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.EdiskRefusedAsRegisteredExclusion]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.EdiskRefusedAsRegisteredExclusion]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:178]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.BadMagicVmdkRefusedNotFlatFallback]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.BadMagicVmdkRefusedNotFlatFallback]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.BadMagicVmdkRefusedNotFlatFallback]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:186]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VmdkSparseExtentReads]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.VmdkSparseExtentReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VmdkSparseExtentReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:276]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VmdkDifferencingChainReadsParent]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.VmdkDifferencingChainReadsParent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VmdkDifferencingChainReadsParent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:311]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VmdkStreamOptimizedRefused]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.VmdkStreamOptimizedRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VmdkStreamOptimizedRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:338]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VhdDynamicDisk]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.VhdDynamicDisk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VhdDynamicDisk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:451]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VhdDifferencingReadsParentOnClearBitmap]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.VhdDifferencingReadsParentOnClearBitmap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VhdDifferencingReadsParentOnClearBitmap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:479]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VhdBadChecksumRefused]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.VhdBadChecksumRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VhdBadChecksumRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:504]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VhdxDynamicReads]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.VhdxDynamicReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VhdxDynamicReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:661]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VhdxLogOverlayServesUnflushedWrites]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.VhdxLogOverlayServesUnflushedWrites]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VhdxLogOverlayServesUnflushedWrites]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:684]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.VhdxDifferencingRefusedPrecisely]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.VhdxDifferencingRefusedPrecisely]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.VhdxDifferencingRefusedPrecisely]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:736]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.Lba0LayoutClassification]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.Lba0LayoutClassification]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.Lba0LayoutClassification]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:751]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.ExtensionClassificationMatchesOgDispatch]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.ExtensionClassificationMatchesOgDispatch]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.ExtensionClassificationMatchesOgDispatch]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:762]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.SessionRoundTripPersistsImagePath]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.SessionRoundTripPersistsImagePath]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.SessionRoundTripPersistsImagePath]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:772]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.QemuVmdkCompat]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.QemuVmdkCompat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.QemuVmdkCompat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:886]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.QemuVhdCompat]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.QemuVhdCompat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.QemuVhdCompat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:911]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Image.QemuVhdxCompat]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Image.QemuVhdxCompat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Image.QemuVhdxCompat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_image.cpp:935]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvmeHealth.PageFieldOffsets]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NvmeHealth.PageFieldOffsets]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvmeHealth.PageFieldOffsets]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:51]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvmeHealth.FullFieldSurface]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NvmeHealth.FullFieldSurface]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvmeHealth.FullFieldSurface]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:69]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvmeHealth.DataUnitsConvertedToBytes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NvmeHealth.DataUnitsConvertedToBytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvmeHealth.DataUnitsConvertedToBytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:83]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvmeHealth.ShortBufferRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NvmeHealth.ShortBufferRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvmeHealth.ShortBufferRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:100]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvmeHealth.NullRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NvmeHealth.NullRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvmeHealth.NullRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:111]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NvmeHealth.PercentageUsedIsU8Width]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NvmeHealth.PercentageUsedIsU8Width]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NvmeHealth.PercentageUsedIsU8Width]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:116]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Nvme.CliTempFractionIsCelsiusDecidegrees]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Nvme.CliTempFractionIsCelsiusDecidegrees]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Nvme.CliTempFractionIsCelsiusDecidegrees]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:133]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.SmartCommandListMatchesImplementation]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.SmartCommandListMatchesImplementation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.SmartCommandListMatchesImplementation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_nvme_health.cpp:167]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.Attr197Raw3IsCaution]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.Attr197Raw3IsCaution]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.Attr197Raw3IsCaution]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:37]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.ValueBelowVendorThresholdIsBad]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.ValueBelowVendorThresholdIsBad]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.ValueBelowVendorThresholdIsBad]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:48]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.Attr5RawAllFFSkipsCautionCheck]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.Attr5RawAllFFSkipsCautionCheck]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.Attr5RawAllFFSkipsCautionCheck]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:63]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.DuplicateAttrIdYieldsUnknown]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.DuplicateAttrIdYieldsUnknown]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.DuplicateAttrIdYieldsUnknown]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:78]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.NoGradedSignalYieldsUnknown]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.NoGradedSignalYieldsUnknown]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.NoGradedSignalYieldsUnknown]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:88]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.SsdAnyAttrBelowThresholdIsBad]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.SsdAnyAttrBelowThresholdIsBad]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.SsdAnyAttrBelowThresholdIsBad]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:107]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.TemperatureNeverAffectsTheDiskGrade]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.TemperatureNeverAffectsTheDiskGrade]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.TemperatureNeverAffectsTheDiskGrade]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:120]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.SsdLifeFamilyGatesAndBoundaries]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.SsdLifeFamilyGatesAndBoundaries]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.SsdLifeFamilyGatesAndBoundaries]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:145]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.SsdLifeBelowVendorThresholdIsBad]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.SsdLifeBelowVendorThresholdIsBad]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.SsdLifeBelowVendorThresholdIsBad]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:193]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.SsdLifeInvertFormReads100MinusRaw]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.SsdLifeInvertFormReads100MinusRaw]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.SsdLifeInvertFormReads100MinusRaw]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:216]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.WdcAttr230LifeFromRawByte]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.WdcAttr230LifeFromRawByte]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.WdcAttr230LifeFromRawByte]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:239]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.Attr1RuleIsSandForceGated]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.Attr1RuleIsSandForceGated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.Attr1RuleIsSandForceGated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:273]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.FailingBitForcesDiskBadButAttrGradesFollowOg]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.FailingBitForcesDiskBadButAttrGradesFollowOg]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.FailingBitForcesDiskBadButAttrGradesFollowOg]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:309]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.NvmeGradeMapping]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.NvmeGradeMapping]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.NvmeGradeMapping]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:327]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.ModelSaysSsdOcList]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.ModelSaysSsdOcList]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.ModelSaysSsdOcList]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:366]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.ClassifyFamilyDecodedVendors]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.ClassifyFamilyDecodedVendors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.ClassifyFamilyDecodedVendors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:384]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.ClassifyFamilyAttributeSignatures]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.ClassifyFamilyAttributeSignatures]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.ClassifyFamilyAttributeSignatures]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:404]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.ClassifyFamilyFormsAndFallbacks]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.ClassifyFamilyFormsAndFallbacks]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.ClassifyFamilyFormsAndFallbacks]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:421]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.MakeGradeInputsAppliesOgDefaults]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.MakeGradeInputsAppliesOgDefaults]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.MakeGradeInputsAppliesOgDefaults]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:444]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.GradeNames]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.GradeNames]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.GradeNames]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_grade.cpp:461]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.CsmiRequestMatchesOgWireFormat]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.CsmiRequestMatchesOgWireFormat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.CsmiRequestMatchesOgWireFormat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:116]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.CsmiIdentifyCarriesZeroLbaAndSmartCarriesMagic]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.CsmiIdentifyCarriesZeroLbaAndSmartCarriesMagic]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.CsmiIdentifyCarriesZeroLbaAndSmartCarriesMagic]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:158]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.CsmiBuilderRejectsUndersizedBuffer]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.CsmiBuilderRejectsUndersizedBuffer]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.CsmiBuilderRejectsUndersizedBuffer]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:175]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.CsmiResponseDataExtractedAtOgOffset]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.CsmiResponseDataExtractedAtOgOffset]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.CsmiResponseDataExtractedAtOgOffset]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:181]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.CsmiTransportUnavailableClassifiesTheOgErrorSet]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.CsmiTransportUnavailableClassifiesTheOgErrorSet]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.CsmiTransportUnavailableClassifiesTheOgErrorSet]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:195]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.CsmiTransportFallsBack]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.CsmiTransportFallsBack]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.CsmiTransportFallsBack]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:205]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.CsmiReadsAttributes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.CsmiReadsAttributes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.CsmiReadsAttributes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:228]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.SmartChainEmptyIsBadParameter]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.SmartChainEmptyIsBadParameter]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.SmartChainEmptyIsBadParameter]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:256]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Health.SmartAttrNameTableCoversTheCuratedIds]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Health.SmartAttrNameTableCoversTheCuratedIds]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Health.SmartAttrNameTableCoversTheCuratedIds]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_health_csmi.cpp:261]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.SingleRunDecode]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RunList.SingleRunDecode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.SingleRunDecode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:9]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.NegativeDelta]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RunList.NegativeDelta]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.NegativeDelta]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:22]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.SparseRun]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RunList.SparseRun]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.SparseRun]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:35]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.TerminatesAndRejectsTruncated]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RunList.TerminatesAndRejectsTruncated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.TerminatesAndRejectsTruncated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:57]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.ZeroLengthRunRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RunList.ZeroLengthRunRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.ZeroLengthRunRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:63]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.MaxClusterGuard]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RunList.MaxClusterGuard]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.MaxClusterGuard]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:69]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.EncodeDecodeRoundTrip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RunList.EncodeDecodeRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.EncodeDecodeRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:76]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.SignedWidthEdgeCases]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RunList.SignedWidthEdgeCases]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.SignedWidthEdgeCases]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:95]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.ExtentCoalesce]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RunList.ExtentCoalesce]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.ExtentCoalesce]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:113]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RunList.Int64MinOffsetRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RunList.Int64MinOffsetRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RunList.Int64MinOffsetRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_runlist.cpp:127]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crc32.KnownVectors]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Crc32.KnownVectors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crc32.KnownVectors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:49]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.ValidTable]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Gpt.ValidTable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.ValidTable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:56]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.BadHeaderCrcDetected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Gpt.BadHeaderCrcDetected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.BadHeaderCrcDetected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:124]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.DeadPrimaryHeaderSalvagesOwnEntries]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Gpt.DeadPrimaryHeaderSalvagesOwnEntries]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.DeadPrimaryHeaderSalvagesOwnEntries]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:426]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.ProtectiveOnlyVolumeRecoversFromBackup]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Gpt.ProtectiveOnlyVolumeRecoversFromBackup]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.ProtectiveOnlyVolumeRecoversFromBackup]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:445]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.HostileSalvageHeaderEntriesOutsideDiskRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Gpt.HostileSalvageHeaderEntriesOutsideDiskRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.HostileSalvageHeaderEntriesOutsideDiskRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:464]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.CanonicalTypeGuidConstantsMatchRealDiskBytes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Gpt.CanonicalTypeGuidConstantsMatchRealDiskBytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.CanonicalTypeGuidConstantsMatchRealDiskBytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:515]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:567]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Gpt.EspMsrLinuxBiosbootLdmClassification]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Gpt.EspMsrLinuxBiosbootLdmClassification]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Gpt.EspMsrLinuxBiosbootLdmClassification]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:578]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Mbr.PlainWithExtended]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Mbr.PlainWithExtended]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Mbr.PlainWithExtended]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:149]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Mbr.EbrLayoutAssignedAndPlainMbrStaysMbr]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Mbr.EbrLayoutAssignedAndPlainMbrStaysMbr]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Mbr.EbrLayoutAssignedAndPlainMbrStaysMbr]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:197]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Mbr.EbrCycleGuard]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Mbr.EbrCycleGuard]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Mbr.EbrCycleGuard]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:240]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Mbr.TornLinkKeepsPriorVolumes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Mbr.TornLinkKeepsPriorVolumes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Mbr.TornLinkKeepsPriorVolumes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:280]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Mbr.TornLinkStatusReported]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Mbr.TornLinkStatusReported]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Mbr.TornLinkStatusReported]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_crc32_gpt.cpp:329]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.BoundsEnforced]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Slice.BoundsEnforced]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.BoundsEnforced]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:35]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.TailReadRefusalPinned]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Slice.TailReadRefusalPinned]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.TailReadRefusalPinned]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:70]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.StartMustBeAligned]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Slice.StartMustBeAligned]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.StartMustBeAligned]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:86]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.RaggedTailAllowed]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Slice.RaggedTailAllowed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.RaggedTailAllowed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:93]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.GeometryRefreshAfterMediaChange]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Slice.GeometryRefreshAfterMediaChange]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.GeometryRefreshAfterMediaChange]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:206]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Slice.GeometryRefreshRefusesInvalidatingChange]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Slice.GeometryRefreshRefusesInvalidatingChange]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Slice.GeometryRefreshRefusesInvalidatingChange]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:236]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DevMap.AddMergeSerialize]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DevMap.AddMergeSerialize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DevMap.AddMergeSerialize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:103]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DevMap.ModuleAttributionRecordedAndNewestWinsOnCoalesce]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DevMap.ModuleAttributionRecordedAndNewestWinsOnCoalesce]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DevMap.ModuleAttributionRecordedAndNewestWinsOnCoalesce]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:125]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DevMap.ModuleNotSerializedWireStaysTrio]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DevMap.ModuleNotSerializedWireStaysTrio]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DevMap.ModuleNotSerializedWireStaysTrio]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_slice_devmap.cpp:159]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Cached.HitMissAndBridging]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Cached.HitMissAndBridging]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Cached.HitMissAndBridging]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_cached.cpp:36]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Cached.BoundsReject]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Cached.BoundsReject]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Cached.BoundsReject]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_cached.cpp:61]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Cached.FailedBlocksAreNotCachedForever]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Cached.FailedBlocksAreNotCachedForever]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Cached.FailedBlocksAreNotCachedForever]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_cached.cpp:68]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Cached.ScanCacheBudgetIsTheRegisteredDivergence]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Cached.ScanCacheBudgetIsTheRegisteredDivergence]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Cached.ScanCacheBudgetIsTheRegisteredDivergence]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_cached.cpp:109]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.AlignedRequestIsTheFastPath]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DirectAlign.AlignedRequestIsTheFastPath]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.AlignedRequestIsTheFastPath]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:15]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.FourKMultipleNotSixtyFourKTakesTheEnvelope]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DirectAlign.FourKMultipleNotSixtyFourKTakesTheEnvelope]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.FourKMultipleNotSixtyFourKTakesTheEnvelope]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:26]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.UnalignedOffsetAndLengthEnvelope]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DirectAlign.UnalignedOffsetAndLengthEnvelope]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.UnalignedOffsetAndLengthEnvelope]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:39]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.MultiBlockEnvelopeCoversTheRequest]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DirectAlign.MultiBlockEnvelopeCoversTheRequest]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.MultiBlockEnvelopeCoversTheRequest]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:50]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.DeviceTailClampIsTheDocumentedException]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DirectAlign.DeviceTailClampIsTheDocumentedException]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.DeviceTailClampIsTheDocumentedException]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:60]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.OneMiBWriterChunkAtAlignedOffsetStaysDirect]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DirectAlign.OneMiBWriterChunkAtAlignedOffsetStaysDirect]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.OneMiBWriterChunkAtAlignedOffsetStaysDirect]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:73]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.OutOfBoundsAndEmptyRequestsFail]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DirectAlign.OutOfBoundsAndEmptyRequestsFail]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.OutOfBoundsAndEmptyRequestsFail]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:84]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.IssuedLengthAlwaysSixtyFourKMultipleExceptTail]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DirectAlign.IssuedLengthAlwaysSixtyFourKMultipleExceptTail]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.IssuedLengthAlwaysSixtyFourKMultipleExceptTail]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:90]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DirectAlign.UnifiedEnvelopeExhaustivePropertyBothDevices]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DirectAlign.UnifiedEnvelopeExhaustivePropertyBothDevices]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DirectAlign.UnifiedEnvelopeExhaustivePropertyBothDevices]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_directalign.cpp:104]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.CleanReadPassesThrough]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Retry.CleanReadPassesThrough]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.CleanReadPassesThrough]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:53]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.FailModeReportsExhaustedAndMapsRange]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Retry.FailModeReportsExhaustedAndMapsRange]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.FailModeReportsExhaustedAndMapsRange]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:65]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.SkipModeZeroFillsOnlyBadSectors]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Retry.SkipModeZeroFillsOnlyBadSectors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.SkipModeZeroFillsOnlyBadSectors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:78]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.UnalignedBadRangeRoundsToWholeSectors]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Retry.UnalignedBadRangeRoundsToWholeSectors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.UnalignedBadRangeRoundsToWholeSectors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:101]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.BacktrackReadSalvagesPartialSector]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Retry.BacktrackReadSalvagesPartialSector]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.BacktrackReadSalvagesPartialSector]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:147]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.BacktrackOffReproducesLegacyExhausted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Retry.BacktrackOffReproducesLegacyExhausted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.BacktrackOffReproducesLegacyExhausted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:166]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.BacktrackStaysAlignedOn4Kn]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Retry.BacktrackStaysAlignedOn4Kn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.BacktrackStaysAlignedOn4Kn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:178]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.BoundsAreEnforcedBeforeInner]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Retry.BoundsAreEnforcedBeforeInner]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.BoundsAreEnforcedBeforeInner]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:192]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.DeterministicFailureNotRetried]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Retry.DeterministicFailureNotRetried]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.DeterministicFailureNotRetried]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:203]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.GeometryForwarded]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Retry.GeometryForwarded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.GeometryForwarded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:213]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.ThreadStampAttributedToRecordedRange]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Retry.ThreadStampAttributedToRecordedRange]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.ThreadStampAttributedToRecordedRange]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:222]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Retry.OwnerFallbackCoversUnstampedWorkerThreads]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Retry.OwnerFallbackCoversUnstampedWorkerThreads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Retry.OwnerFallbackCoversUnstampedWorkerThreads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:242]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Errmod.GuardSetsAndRestoresTheThreadStamp]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Errmod.GuardSetsAndRestoresTheThreadStamp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Errmod.GuardSetsAndRestoresTheThreadStamp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_retry.cpp:273]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pool.RunsAllItemsExactlyOnce]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pool.RunsAllItemsExactlyOnce]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pool.RunsAllItemsExactlyOnce]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:22]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pool.AbortStopsWork]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pool.AbortStopsWork]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pool.AbortStopsWork]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:36]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pool.ExternalCancelTokenAdapter]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pool.ExternalCancelTokenAdapter]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pool.ExternalCancelTokenAdapter]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:50]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pool.ExceptionsNeverCrossThreadEdge]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pool.ExceptionsNeverCrossThreadEdge]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pool.ExceptionsNeverCrossThreadEdge]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:58]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pool.WaitIdleAndPending]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pool.WaitIdleAndPending]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pool.WaitIdleAndPending]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:71]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pool.CancelPoolWatcherDrainsQueueWithinOneTick]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pool.CancelPoolWatcherDrainsQueueWithinOneTick]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pool.CancelPoolWatcherDrainsQueueWithinOneTick]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:273]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pool.CancelPoolWatcherNullTokenIsInert]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pool.CancelPoolWatcherNullTokenIsInert]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pool.CancelPoolWatcherNullTokenIsInert]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:312]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ParallelCarve.MatchesSingleThreaded]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ParallelCarve.MatchesSingleThreaded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ParallelCarve.MatchesSingleThreaded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:159]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ParallelRemnants.SameRecordsAsSequential]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ParallelRemnants.SameRecordsAsSequential]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ParallelRemnants.SameRecordsAsSequential]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_parallel.cpp:249]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NodeTree.AddRootsAndFiles]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NodeTree.AddRootsAndFiles]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NodeTree.AddRootsAndFiles]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_node_tree.cpp:19]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NodeTree.HardLinkSharesProps]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NodeTree.HardLinkSharesProps]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NodeTree.HardLinkSharesProps]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_node_tree.cpp:41]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NodeTree.ExtentDedupIndex]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NodeTree.ExtentDedupIndex]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NodeTree.ExtentDedupIndex]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_node_tree.cpp:54]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NodeTree.ResetFreesClean]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NodeTree.ResetFreesClean]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NodeTree.ResetFreesClean]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_node_tree.cpp:67]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.MonotonicMaxNeverDowngrades]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Grading.MonotonicMaxNeverDowngrades]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.MonotonicMaxNeverDowngrades]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:58]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.Grade2Sticky]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Grading.Grade2Sticky]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.Grade2Sticky]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:73]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.DefaultGradeIsAverageAtEmit]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Grading.DefaultGradeIsAverageAtEmit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.DefaultGradeIsAverageAtEmit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:87]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.SizeFloorIs1]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Grading.SizeFloorIs1]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.SizeFloorIs1]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:99]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.EmailCategoryFastPath]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Grading.EmailCategoryFastPath]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.EmailCategoryFastPath]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:111]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.ExistingFilesHighDeletedNot]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Grading.ExistingFilesHighDeletedNot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.ExistingFilesHighDeletedNot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:126]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.SmallFileUnder1024High]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Grading.SmallFileUnder1024High]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.SmallFileUnder1024High]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:138]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.NtfsDeviceIndexRuleUpgrades]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Grading.NtfsDeviceIndexRuleUpgrades]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.NtfsDeviceIndexRuleUpgrades]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:154]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.ExtensionTableGatesDeepValidation]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Grading.ExtensionTableGatesDeepValidation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.ExtensionTableGatesDeepValidation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:166]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.TwinAddressFlagsBoth]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Grading.TwinAddressFlagsBoth]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.TwinAddressFlagsBoth]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:189]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.TableMembershipAndCount]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Grading.TableMembershipAndCount]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.TableMembershipAndCount]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:212]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.RegradeOnlyGrade1]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Grading.RegradeOnlyGrade1]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.RegradeOnlyGrade1]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:225]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.AssistantCountersMixedFixture]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Grading.AssistantCountersMixedFixture]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.AssistantCountersMixedFixture]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:245]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Grading.SessionRoundTripPreservesGrade]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Grading.SessionRoundTripPreservesGrade]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Grading.SessionRoundTripPreservesGrade]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:275]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.ContainsAssistantStatistics]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Report.ContainsAssistantStatistics]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.ContainsAssistantStatistics]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_grading.cpp:353]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.EscapingStrictJson]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Report.EscapingStrictJson]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.EscapingStrictJson]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:97]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.SchemaShapeAndDeterminism]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Report.SchemaShapeAndDeterminism]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.SchemaShapeAndDeterminism]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:119]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.WriterOutcomesAndHashHonesty]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Report.WriterOutcomesAndHashHonesty]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.WriterOutcomesAndHashHonesty]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:178]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.MultiExtentSpannedVolumeRefused]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Report.MultiExtentSpannedVolumeRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.MultiExtentSpannedVolumeRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:253]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.DestFullFastFailStopsAttempting]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Report.DestFullFastFailStopsAttempting]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.DestFullFastFailStopsAttempting]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:311]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.DestFullPreCheckRefusesUpfront]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Report.DestFullPreCheckRefusesUpfront]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.DestFullPreCheckRefusesUpfront]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:352]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.SalvagedOutcomeMarksZeroFilledCommits]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Report.SalvagedOutcomeMarksZeroFilledCommits]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.SalvagedOutcomeMarksZeroFilledCommits]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:384]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Report.EmitsCiSchemaSample]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Report.EmitsCiSchemaSample]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Report.EmitsCiSchemaSample]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_report.cpp:454]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.ScopeDeltaRegisteredWithRevisitTrigger]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Enumerate.ScopeDeltaRegisteredWithRevisitTrigger]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.ScopeDeltaRegisteredWithRevisitTrigger]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_win.cpp:21]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.BusyDiskMetadataRetryShipsInQueryDisk]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Enumerate.BusyDiskMetadataRetryShipsInQueryDisk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.BusyDiskMetadataRetryShipsInQueryDisk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_win.cpp:36]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.ScopeContractDocumentedInHeader]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Enumerate.ScopeContractDocumentedInHeader]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.ScopeContractDocumentedInHeader]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_win.cpp:56]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.SetupApiMerge]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Enumerate.SetupApiMerge]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.SetupApiMerge]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_merge.cpp:44]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.DedupByInstancePath]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Enumerate.DedupByInstancePath]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.DedupByInstancePath]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_merge.cpp:73]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.MergeWithEmptyInterfaceStackKeepsLoop]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Enumerate.MergeWithEmptyInterfaceStackKeepsLoop]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.MergeWithEmptyInterfaceStackKeepsLoop]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_merge.cpp:96]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Enumerate.MergeWithEmptyLoopTakesAllInterfaces]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Enumerate.MergeWithEmptyLoopTakesAllInterfaces]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Enumerate.MergeWithEmptyLoopTakesAllInterfaces]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_enumerate_merge.cpp:109]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.NonresidentAttrlistOnMft]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.NonresidentAttrlistOnMft]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.NonresidentAttrlistOnMft]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:805]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.NestedAttrlistChase]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.NestedAttrlistChase]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.NestedAttrlistChase]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:854]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.ResidentAdsCarriesContent]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.ResidentAdsCarriesContent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.ResidentAdsCarriesContent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:918]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.AdsAttachesToFirstLink]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.AdsAttachesToFirstLink]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.AdsAttachesToFirstLink]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:959]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.UsnJrnlHarvest]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.UsnJrnlHarvest]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.UsnJrnlHarvest]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1063]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.UsnJrnlSparseV3Resync]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.UsnJrnlSparseV3Resync]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.UsnJrnlSparseV3Resync]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1171]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.ShiftedHeaderSalvage]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.ShiftedHeaderSalvage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.ShiftedHeaderSalvage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1310]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.EndToEndWalk]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.EndToEndWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.EndToEndWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1347]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.HardLinkSingleLinkMatchesOg]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.HardLinkSingleLinkMatchesOg]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.HardLinkSingleLinkMatchesOg]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1533]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.HardLinkFanOutOptIn]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.HardLinkFanOutOptIn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.HardLinkFanOutOptIn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1572]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RecycleBinMapping]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.RecycleBinMapping]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RecycleBinMapping]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1606]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RemnantScanFindsDetachedRecord]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.RemnantScanFindsDetachedRecord]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RemnantScanFindsDetachedRecord]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1703]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.ViabilityProbeUsesDataSize]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.ViabilityProbeUsesDataSize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.ViabilityProbeUsesDataSize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1731]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.ViabilityBoundaryExactFastPath]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.ViabilityBoundaryExactFastPath]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.ViabilityBoundaryExactFastPath]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1786]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.ExtensionRecordsNotCountedDamaged]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.ExtensionRecordsNotCountedDamaged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.ExtensionRecordsNotCountedDamaged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1835]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.CompressedDataRecoversRealContent]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.CompressedDataRecoversRealContent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.CompressedDataRecoversRealContent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1879]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.Lznt1MsXcaDocumentedVector]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.Lznt1MsXcaDocumentedVector]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.Lznt1MsXcaDocumentedVector]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:1980]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RemnantScanReconstructsDataRunlist]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.RemnantScanReconstructsDataRunlist]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RemnantScanReconstructsDataRunlist]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2010]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RemnantScanReconstructsResidentData]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.RemnantScanReconstructsResidentData]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RemnantScanReconstructsResidentData]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2046]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RemnantScanCompressedStreamStaysMetadataOnly]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.RemnantScanCompressedStreamStaysMetadataOnly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RemnantScanCompressedStreamStaysMetadataOnly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2076]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.OverwrittenHonestModeKeepsAndGrades]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.OverwrittenHonestModeKeepsAndGrades]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.OverwrittenHonestModeKeepsAndGrades]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2110]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.HardLinkDestroyRuleInteraction]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.HardLinkDestroyRuleInteraction]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.HardLinkDestroyRuleInteraction]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2155]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BootNoPow2RecordSizeAcceptedAtDecodedValue]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.BootNoPow2RecordSizeAcceptedAtDecodedValue]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BootNoPow2RecordSizeAcceptedAtDecodedValue]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2320]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BootRecordSizeZeroRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.BootRecordSizeZeroRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BootRecordSizeZeroRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2338]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BootNegativeExponentOverflowRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.BootNegativeExponentOverflowRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BootNegativeExponentOverflowRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2352]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BootUsableLargeRecordSizeKept]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.BootUsableLargeRecordSizeKept]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BootUsableLargeRecordSizeKept]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2371]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RemnantReachableUnderTornDbr]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.RemnantReachableUnderTornDbr]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RemnantReachableUnderTornDbr]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2384]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.MirrorCoversRunlistSize]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.MirrorCoversRunlistSize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.MirrorCoversRunlistSize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2487]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.MirrorResidentDataRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.MirrorResidentDataRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.MirrorResidentDataRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2528]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.UserFileNamedDollarNotHidden]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.UserFileNamedDollarNotHidden]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.UserFileNamedDollarNotHidden]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2551]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.MainWalkKeepsOverwrittenDeleted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.MainWalkKeepsOverwrittenDeleted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.MainWalkKeepsOverwrittenDeleted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2833]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.GuidDumpPassRecoversReusedRecord]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.GuidDumpPassRecoversReusedRecord]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.GuidDumpPassRecoversReusedRecord]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2871]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.GuidDumpDestroyRules]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.GuidDumpDestroyRules]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.GuidDumpDestroyRules]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2889]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.SizeZeroRescueFromMftCopy]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.SizeZeroRescueFromMftCopy]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.SizeZeroRescueFromMftCopy]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2907]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.SizeZeroRescueSameNameMerge]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.SizeZeroRescueSameNameMerge]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.SizeZeroRescueSameNameMerge]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2925]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.GuidDumpStrideIsRecordAligned]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.GuidDumpStrideIsRecordAligned]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.GuidDumpStrideIsRecordAligned]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2945]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.GuidDumpLargeRecordParsedFully]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.GuidDumpLargeRecordParsedFully]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.GuidDumpLargeRecordParsedFully]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:2980]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.MainWalkKeepsSystemFlaggedUserFile]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.MainWalkKeepsSystemFlaggedUserFile]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.MainWalkKeepsSystemFlaggedUserFile]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3090]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.OrphanIndxBlockHarvestsEntries]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.OrphanIndxBlockHarvestsEntries]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.OrphanIndxBlockHarvestsEntries]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3249]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.IndxHiddenSystemEntryRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.IndxHiddenSystemEntryRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.IndxHiddenSystemEntryRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3289]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.IndxSlackNeverHarvested]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.IndxSlackNeverHarvested]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.IndxSlackNeverHarvested]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3313]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RemnantDirRecordHarvestsIndexRootChildren]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.RemnantDirRecordHarvestsIndexRootChildren]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RemnantDirRecordHarvestsIndexRootChildren]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3344]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.AdsDroppedInParityMode]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.AdsDroppedInParityMode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.AdsDroppedInParityMode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3384]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.AdsEmittedByDefaultSuperset]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.AdsEmittedByDefaultSuperset]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.AdsEmittedByDefaultSuperset]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3432]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.AttributesFromSIWhenAvailable]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.AttributesFromSIWhenAvailable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.AttributesFromSIWhenAvailable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3454]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.AttributesFallBackToFileNameWhenSIAbsent]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.AttributesFallBackToFileNameWhenSIAbsent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.AttributesFallBackToFileNameWhenSIAbsent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3496]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.AttributesPreserveOfflineBit]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.AttributesPreserveOfflineBit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.AttributesPreserveOfflineBit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3536]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.ShortNameFieldRoundtrips]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.ShortNameFieldRoundtrips]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.ShortNameFieldRoundtrips]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3576]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.UsnHarvestTrueDeleteReason0x80020000]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.UsnHarvestTrueDeleteReason0x80020000]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.UsnHarvestTrueDeleteReason0x80020000]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3677]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.UsnHarvestRejectsCompressionChangeOnly]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.UsnHarvestRejectsCompressionChangeOnly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.UsnHarvestRejectsCompressionChangeOnly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3741]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.UsnHarvestCapMinesTail]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.UsnHarvestCapMinesTail]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.UsnHarvestCapMinesTail]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3802]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.DiskMapFreeCellsFromRealBitmap]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.DiskMapFreeCellsFromRealBitmap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.DiskMapFreeCellsFromRealBitmap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:3944]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.MultiHopAttrlistOgParityStopsAtDepth1]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.MultiHopAttrlistOgParityStopsAtDepth1]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.MultiHopAttrlistOgParityStopsAtDepth1]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4069]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.DeletedAttrlistChaseFollowsChildrenAtDepth1]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.DeletedAttrlistChaseFollowsChildrenAtDepth1]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.DeletedAttrlistChaseFollowsChildrenAtDepth1]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4123]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.TornPrimaryBackupBootScans]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.TornPrimaryBackupBootScans]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.TornPrimaryBackupBootScans]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4187]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.TornPrimaryShapeBackupBootScans]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.TornPrimaryShapeBackupBootScans]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.TornPrimaryShapeBackupBootScans]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4222]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BackupBadFallsBackToRefusal]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.BackupBadFallsBackToRefusal]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BackupBadFallsBackToRefusal]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4243]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.BootFromBackupFlagged]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.BootFromBackupFlagged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.BootFromBackupFlagged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4259]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.SpoofedOemRejectedEarly]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.SpoofedOemRejectedEarly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.SpoofedOemRejectedEarly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4271]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[NtfsVolume.RecycleBinScopeOutOfBinDollarFilesStayRaw]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=NtfsVolume.RecycleBinScopeOutOfBinDollarFilesStayRaw]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[NtfsVolume.RecycleBinScopeOutOfBinDollarFilesStayRaw]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ntfs_volume.cpp:4308]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.FreeMapExcludesVirtualClustersAndMatchesFAT]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.FreeMapExcludesVirtualClustersAndMatchesFAT]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.FreeMapExcludesVirtualClustersAndMatchesFAT]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:198]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat16EndToEnd]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.Fat16EndToEnd]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat16EndToEnd]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:244]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.LfnCorruptFallsBackToShort]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.LfnCorruptFallsBackToShort]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.LfnCorruptFallsBackToShort]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:305]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat32RootChainAndBackupBoot]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.Fat32RootChainAndBackupBoot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat32RootChainAndBackupBoot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:324]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat16HasNoBackupBootFallback]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.Fat16HasNoBackupBootFallback]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat16HasNoBackupBootFallback]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:407]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat32BackupBootFallback]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.Fat32BackupBootFallback]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat32BackupBootFallback]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:423]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.ChainCycleIsCut]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.ChainCycleIsCut]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.ChainCycleIsCut]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:471]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat12ChainsCanonicalEocAndBad]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.Fat12ChainsCanonicalEocAndBad]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat12ChainsCanonicalEocAndBad]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:574]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedLfnFragmentOrder]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedLfnFragmentOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedLfnFragmentOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:617]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedLfnRealWorldChecksumRecoversFullName]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedLfnRealWorldChecksumRecoversFullName]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedLfnRealWorldChecksumRecoversFullName]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:667]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedLfnSingleSlotChecksumRecovery]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedLfnSingleSlotChecksumRecovery]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedLfnSingleSlotChecksumRecovery]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:704]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedLfnTornChecksumKeepsHonestFallback]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedLfnTornChecksumKeepsHonestFallback]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedLfnTornChecksumKeepsHonestFallback]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:734]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.LfnDeletedChecksumBijection]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.LfnDeletedChecksumBijection]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.LfnDeletedChecksumBijection]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:769]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedUnlinkedChainGetsContiguousRun]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedUnlinkedChainGetsContiguousRun]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedUnlinkedChainGetsContiguousRun]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:805]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedUnlinkedChainTruncatesAtReusedCluster]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedUnlinkedChainTruncatesAtReusedCluster]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedUnlinkedChainTruncatesAtReusedCluster]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:839]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DeletedLinkedChainStillPrefersFatTruth]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.DeletedLinkedChainStillPrefersFatTruth]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DeletedLinkedChainStillPrefersFatTruth]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:870]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat32EocMask24AndAlternateFamiliesEndClean]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.Fat32EocMask24AndAlternateFamiliesEndClean]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat32EocMask24AndAlternateFamiliesEndClean]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:897]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat16BadClusterStillCanonical]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.Fat16BadClusterStillCanonical]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat16BadClusterStillCanonical]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:961]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.LargeFat32TableAccepted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.LargeFat32TableAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.LargeFat32TableAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1037]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.SmallFat16TableDeclined]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.SmallFat16TableDeclined]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.SmallFat16TableDeclined]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1085]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.MirrorShiftedCopyRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.MirrorShiftedCopyRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.MirrorShiftedCopyRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1131]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.MirrorCrossLinkRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.MirrorCrossLinkRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.MirrorCrossLinkRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1182]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.MirrorOnlyScanFat1Unreadable]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.MirrorOnlyScanFat1Unreadable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.MirrorOnlyScanFat1Unreadable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1212]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.CrossLinkedFat1RejectedAndFat2Used]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.CrossLinkedFat1RejectedAndFat2Used]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.CrossLinkedFat1RejectedAndFat2Used]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1261]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.MediaByteCanonicalEnforcedFat16]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.MediaByteCanonicalEnforcedFat16]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.MediaByteCanonicalEnforcedFat16]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1296]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.InvalidDateYieldsZeroFileTime]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.InvalidDateYieldsZeroFileTime]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.InvalidDateYieldsZeroFileTime]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1333]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.InvalidMonthYieldsZeroFileTime]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.InvalidMonthYieldsZeroFileTime]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.InvalidMonthYieldsZeroFileTime]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1354]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.InvalidDayYieldsZeroFileTime]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.InvalidDayYieldsZeroFileTime]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.InvalidDayYieldsZeroFileTime]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1371]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.ValidDateYieldsCorrectFileTime]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.ValidDateYieldsCorrectFileTime]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.ValidDateYieldsCorrectFileTime]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1388]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.FlavorBoundary65524IsFat32]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.FlavorBoundary65524IsFat32]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.FlavorBoundary65524IsFat32]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1407]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.BootStringGateDeclinesTornStringVolume]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.BootStringGateDeclinesTornStringVolume]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.BootStringGateDeclinesTornStringVolume]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1418]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.BootStringCaseInsensitiveAccepted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.BootStringCaseInsensitiveAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.BootStringCaseInsensitiveAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1434]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat16LowRootEntriesDeclined]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.Fat16LowRootEntriesDeclined]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat16LowRootEntriesDeclined]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1448]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.Fat16RootEntriesNotSectorMultipleDeclined]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.Fat16RootEntriesNotSectorMultipleDeclined]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.Fat16RootEntriesNotSectorMultipleDeclined]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1462]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.NumFatsThreeDeclined]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.NumFatsThreeDeclined]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.NumFatsThreeDeclined]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1476]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.SpcDomainMatchesOgSet]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.SpcDomainMatchesOgSet]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.SpcDomainMatchesOgSet]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1490]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.FatBps1536Accepted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.FatBps1536Accepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.FatBps1536Accepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1518]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.FatBps32768CeilingAndNonMultipleRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.FatBps32768CeilingAndNonMultipleRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.FatBps32768CeilingAndNonMultipleRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1536]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.FatZeroNumFatsRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.FatZeroNumFatsRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.FatZeroNumFatsRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1557]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.FatClusterCountOverflowRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.FatClusterCountOverflowRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.FatClusterCountOverflowRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1571]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.DateSanityRejectedInStrictMode]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.DateSanityRejectedInStrictMode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.DateSanityRejectedInStrictMode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1590]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.BadPrimaryServedWithoutMirrorConsult]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.BadPrimaryServedWithoutMirrorConsult]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.BadPrimaryServedWithoutMirrorConsult]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1625]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.PrimaryFreeStillConsultsMirror]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.PrimaryFreeStillConsultsMirror]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.PrimaryFreeStillConsultsMirror]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1655]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.RecyclePairingRestoresOriginalName]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.RecyclePairingRestoresOriginalName]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.RecyclePairingRestoresOriginalName]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1754]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.RecycleLegacyRoots]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.RecycleLegacyRoots]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.RecycleLegacyRoots]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1787]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FatEngine.NonRecycleDollarFilesStayRaw]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FatEngine.NonRecycleDollarFilesStayRaw]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FatEngine.NonRecycleDollarFilesStayRaw]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_fat_volume.cpp:1805]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.TimestampsMatchOgLocalAsUtc]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.TimestampsMatchOgLocalAsUtc]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.TimestampsMatchOgLocalAsUtc]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:424]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.TimestampsZeroTzPassThrough]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.TimestampsZeroTzPassThrough]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.TimestampsZeroTzPassThrough]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:445]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.TimestampsMatchExplorerDisplay]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.TimestampsMatchExplorerDisplay]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.TimestampsMatchExplorerDisplay]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:459]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.TimestampsWestOffsetAlsoIgnored]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.TimestampsWestOffsetAlsoIgnored]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.TimestampsWestOffsetAlsoIgnored]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:475]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.FreeMapExcludesVirtualClustersAndMatchesBitmap]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.FreeMapExcludesVirtualClustersAndMatchesBitmap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.FreeMapExcludesVirtualClustersAndMatchesBitmap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:490]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.TornBitmapReadYieldsNoFreeMap]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.TornBitmapReadYieldsNoFreeMap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.TornBitmapReadYieldsNoFreeMap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:561]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.EndToEnd]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.EndToEnd]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.EndToEnd]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:584]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.UpcaseTableChecksumValidated]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.UpcaseTableChecksumValidated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.UpcaseTableChecksumValidated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:671]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.UpcaseLengthSanity]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.UpcaseLengthSanity]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.UpcaseLengthSanity]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:712]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.FreeMapProducerMatchesSpecBitMapping]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.FreeMapProducerMatchesSpecBitMapping]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.FreeMapProducerMatchesSpecBitMapping]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:753]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.SetChecksumMismatchGradesExcellentInOgProfile]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.SetChecksumMismatchGradesExcellentInOgProfile]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.SetChecksumMismatchGradesExcellentInOgProfile]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:816]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.SetChecksumMismatchMarksDamaged]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.SetChecksumMismatchMarksDamaged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.SetChecksumMismatchMarksDamaged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:837]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.InvalidFirstClusterRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.InvalidFirstClusterRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.InvalidFirstClusterRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:864]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.EocFamilyTerminatesChain]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.EocFamilyTerminatesChain]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.EocFamilyTerminatesChain]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:910]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.FreeEntryContiguousSalvage]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.FreeEntryContiguousSalvage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.FreeEntryContiguousSalvage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:944]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.BootGatesMatchOg]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.BootGatesMatchOg]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.BootGatesMatchOg]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:980]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.UpcaseAlgorithmMatchesRealMkfsExfat]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.UpcaseAlgorithmMatchesRealMkfsExfat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.UpcaseAlgorithmMatchesRealMkfsExfat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1196]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.LargeFatFullChains]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.LargeFatFullChains]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.LargeFatFullChains]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1215]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.LargeFatDirectoryWalk]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.LargeFatDirectoryWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.LargeFatDirectoryWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1255]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.StreamReservedPartiallyNonzeroPassesOgByteOr]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.StreamReservedPartiallyNonzeroPassesOgByteOr]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.StreamReservedPartiallyNonzeroPassesOgByteOr]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1286]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.StreamReservedAllBytesNonzeroStillDamaged]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.StreamReservedAllBytesNonzeroStillDamaged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.StreamReservedAllBytesNonzeroStillDamaged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1325]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.StreamReserved1ByteStrictlyZero]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.StreamReserved1ByteStrictlyZero]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.StreamReserved1ByteStrictlyZero]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1351]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.DeletedSetChecksumRejectsTornSet]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.DeletedSetChecksumRejectsTornSet]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.DeletedSetChecksumRejectsTornSet]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1378]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.DeletedSetOracleReassembly]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.DeletedSetOracleReassembly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.DeletedSetOracleReassembly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1407]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.LiveCompleteSetBadChecksumUngraded]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.LiveCompleteSetBadChecksumUngraded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.LiveCompleteSetBadChecksumUngraded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1453]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.EmptyFileSuppressedLikeOg]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.EmptyFileSuppressedLikeOg]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.EmptyFileSuppressedLikeOg]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1506]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.GarbageTimestampRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.GarbageTimestampRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.GarbageTimestampRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1541]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.SecondaryCountCapped]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.SecondaryCountCapped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.SecondaryCountCapped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1594]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.BadNameFlagsRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.BadNameFlagsRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.BadNameFlagsRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1630]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.FileReservedNonzeroRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.FileReservedNonzeroRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.FileReservedNonzeroRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1679]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.ValidBoundaryTimestampsStillPass]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.ValidBoundaryTimestampsStillPass]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.ValidBoundaryTimestampsStillPass]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1715]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.NameUtf16TableInvalidCharRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.NameUtf16TableInvalidCharRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.NameUtf16TableInvalidCharRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1772]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.NameUtf16TableAllowedCharAccepted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.NameUtf16TableAllowedCharAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.NameUtf16TableAllowedCharAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1792]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExfatEngine.RecyclePairingMirrored]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExfatEngine.RecyclePairingMirrored]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExfatEngine.RecyclePairingMirrored]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_exfat_volume.cpp:1815]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.ExtentTreeDepth3]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.ExtentTreeDepth3]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.ExtentTreeDepth3]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:270]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.SuperblockValidation]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.SuperblockValidation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.SuperblockValidation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:407]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.EndToEndWalk]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.EndToEndWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.EndToEndWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:441]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.BadMagicRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.BadMagicRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.BadMagicRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:494]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.UnixTimeConversion]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.UnixTimeConversion]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.UnixTimeConversion]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:507]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.JournalSalvage]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.JournalSalvage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.JournalSalvage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:512]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.InodeSizeOffsetIsSpecTrue]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.InodeSizeOffsetIsSpecTrue]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.InodeSizeOffsetIsSpecTrue]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:541]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.JournalUncommittedTailSurfaces]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.JournalUncommittedTailSurfaces]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.JournalUncommittedTailSurfaces]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:557]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.JournalUnreferencedDirBlockSalvaged]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.JournalUnreferencedDirBlockSalvaged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.JournalUnreferencedDirBlockSalvaged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:607]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.JournalFirstBlockNotJsbNoSalvage]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.JournalFirstBlockNotJsbNoSalvage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.JournalFirstBlockNotJsbNoSalvage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:641]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.CorruptRecLenSkipsEntryNotBlock]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.CorruptRecLenSkipsEntryNotBlock]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.CorruptRecLenSkipsEntryNotBlock]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:674]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.InodeZeroHoleContinues]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.InodeZeroHoleContinues]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.InodeZeroHoleContinues]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:711]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.FileTypeGateSkipsInvalid]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.FileTypeGateSkipsInvalid]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.FileTypeGateSkipsInvalid]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:754]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.NonAsciiNamesDecodedNotByteLifted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.NonAsciiNamesDecodedNotByteLifted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.NonAsciiNamesDecodedNotByteLifted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:797]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.MetaBgVolumeRecoversAllGroups]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.MetaBgVolumeRecoversAllGroups]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.MetaBgVolumeRecoversAllGroups]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1023]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.MetaBgDeletedSweepReachesDistributedGroups]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.MetaBgDeletedSweepReachesDistributedGroups]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.MetaBgDeletedSweepReachesDistributedGroups]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1045]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.MetaBgDescriptorBlockReadOnce]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.MetaBgDescriptorBlockReadOnce]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.MetaBgDescriptorBlockReadOnce]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1068]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.MetaBgFirstMetaBgFrontRegion]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.MetaBgFirstMetaBgFrontRegion]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.MetaBgFirstMetaBgFrontRegion]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1089]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.MetaBgSuperblockFieldsParsed]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.MetaBgSuperblockFieldsParsed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.MetaBgSuperblockFieldsParsed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1109]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.BackupSuperblockRescueWhenPrimaryDestroyed]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.BackupSuperblockRescueWhenPrimaryDestroyed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.BackupSuperblockRescueWhenPrimaryDestroyed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1202]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.BackupRescueFallsBackToBackupGdt]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.BackupRescueFallsBackToBackupGdt]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.BackupRescueFallsBackToBackupGdt]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1225]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.BackupRescueWholeDiskRebase]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.BackupRescueWholeDiskRebase]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.BackupRescueWholeDiskRebase]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1243]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.BadMagicWithoutBackupsStillRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.BadMagicWithoutBackupsStillRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.BadMagicWithoutBackupsStillRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1277]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.JournalSalvagedDirReparented]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.JournalSalvagedDirReparented]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.JournalSalvagedDirReparented]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1310]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.JournalUnreferencedOrphanFlatWithRealName]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.JournalUnreferencedOrphanFlatWithRealName]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.JournalUnreferencedOrphanFlatWithRealName]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1369]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.TwoPassJournalClaimInteraction]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.TwoPassJournalClaimInteraction]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.TwoPassJournalClaimInteraction]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1402]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.CatchBufferBacktrackSalvage]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.CatchBufferBacktrackSalvage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.CatchBufferBacktrackSalvage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1489]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.BacktrackOffZeroFillsTheWindow]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.BacktrackOffZeroFillsTheWindow]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.BacktrackOffZeroFillsTheWindow]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1527]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.ZeroFillOnlyWhenBacktrackAlsoFails]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.ZeroFillOnlyWhenBacktrackAlsoFails]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.ZeroFillOnlyWhenBacktrackAlsoFails]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1566]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.NanosecondsPreservedFromExtraField]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.NanosecondsPreservedFromExtraField]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.NanosecondsPreservedFromExtraField]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1634]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ext4Engine.InvalidUtf8NameCarriesRawBytes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ext4Engine.InvalidUtf8NameCarriesRawBytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ext4Engine.InvalidUtf8NameCarriesRawBytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ext4_volume.cpp:1714]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.VolumeHeaderValidation]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.VolumeHeaderValidation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.VolumeHeaderValidation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:209]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.EndToEndWalk]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.EndToEndWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.EndToEndWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:242]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.DecmpfsInlineAndRsrcContentRecovered]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.DecmpfsInlineAndRsrcContentRecovered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.DecmpfsInlineAndRsrcContentRecovered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:366]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.DecmpfsDisabledLeavesEmptyShell]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.DecmpfsDisabledLeavesEmptyShell]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.DecmpfsDisabledLeavesEmptyShell]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:500]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.MacTimeConversion]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.MacTimeConversion]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.MacTimeConversion]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:547]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.BadSignatureRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.BadSignatureRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.BadSignatureRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:554]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.LeafNodeLayoutMatchesOnDiskFormat]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.LeafNodeLayoutMatchesOnDiskFormat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.LeafNodeLayoutMatchesOnDiskFormat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:572]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.DotfileAndPrivateDataFiltered]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.DotfileAndPrivateDataFiltered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.DotfileAndPrivateDataFiltered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:607]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.PrdfRecordFolderized]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.PrdfRecordFolderized]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.PrdfRecordFolderized]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:661]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.RecordBit29SelectsResourceFork]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.RecordBit29SelectsResourceFork]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.RecordBit29SelectsResourceFork]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:709]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.LargeBlockSizeAccepted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.LargeBlockSizeAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.LargeBlockSizeAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:776]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.ExtentsOverflowRecordResolvesFragmentedFork]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.ExtentsOverflowRecordResolvesFragmentedFork]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.ExtentsOverflowRecordResolvesFragmentedFork]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:876]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalHarvestEmitsDeletedRecords]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalHarvestEmitsDeletedRecords]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalHarvestEmitsDeletedRecords]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1018]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalDifferentialDifferentFileIdKept]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalDifferentialDifferentFileIdKept]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalDifferentialDifferentFileIdKept]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1066]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalBadHeaderGates]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalBadHeaderGates]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalBadHeaderGates]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1096]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalWrapHandled]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalWrapHandled]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalWrapHandled]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1127]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalHarvestDisabledByFlag]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalHarvestDisabledByFlag]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalHarvestDisabledByFlag]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1174]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalSameKeyDifferentFileIdBothKept]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalSameKeyDifferentFileIdBothKept]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalSameKeyDifferentFileIdBothKept]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1193]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalAttrDecmpfsFallbackFeedsEmptyForkFile]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalAttrDecmpfsFallbackFeedsEmptyForkFile]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalAttrDecmpfsFallbackFeedsEmptyForkFile]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1230]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalRequiresVolumeJournaledBit]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalRequiresVolumeJournaledBit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalRequiresVolumeJournaledBit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1299]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalInfoBlockHasNoMagicField]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalInfoBlockHasNoMagicField]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalInfoBlockHasNoMagicField]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1319]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalNodeImageRejectsInconsistentOffsetTable]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalNodeImageRejectsInconsistentOffsetTable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalNodeImageRejectsInconsistentOffsetTable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1341]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalFallbackSalvageWhenHeaderInvalid]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalFallbackSalvageWhenHeaderInvalid]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalFallbackSalvageWhenHeaderInvalid]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1367]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[HfsPlus.JournalFallbackRequiresNodeShapedStart]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=HfsPlus.JournalFallbackRequiresNodeShapedStart]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[HfsPlus.JournalFallbackRequiresNodeShapedStart]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_hfsplus_volume.cpp:1403]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.SuperblockValidation]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.SuperblockValidation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.SuperblockValidation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:327]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.NonPow2NodesizeRejectedRegistered]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.NonPow2NodesizeRejectedRegistered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.NonPow2NodesizeRejectedRegistered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:364]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.EndToEndWalk]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.EndToEndWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.EndToEndWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:383]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.TwoSubvolumesRootSeparately]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.TwoSubvolumesRootSeparately]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.TwoSubvolumesRootSeparately]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:444]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.SubvolumeNamedFromRootRef]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.SubvolumeNamedFromRootRef]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.SubvolumeNamedFromRootRef]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:618]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.FileAttributeNormalBit]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.FileAttributeNormalBit]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.FileAttributeNormalBit]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:700]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.BadMagicRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.BadMagicRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.BadMagicRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:729]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.PackedDirItemsInOneItem]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.PackedDirItemsInOneItem]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.PackedDirItemsInOneItem]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:739]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.MirrorSelectionNewestGeneration]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.MirrorSelectionNewestGeneration]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.MirrorSelectionNewestGeneration]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:792]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.InternalKeyPtrWalk]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.InternalKeyPtrWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.InternalKeyPtrWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:826]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.InlineAndCompressedExtents]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.InlineAndCompressedExtents]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.InlineAndCompressedExtents]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:936]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.NonAsciiNamesDecodedNotByteLifted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.NonAsciiNamesDecodedNotByteLifted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.NonAsciiNamesDecodedNotByteLifted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1082]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.Latin1NameRoundTripsRawBytes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.Latin1NameRoundTripsRawBytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.Latin1NameRoundTripsRawBytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1152]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.XattrDosAttribApplied]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.XattrDosAttribApplied]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.XattrDosAttribApplied]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1239]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.UserXattrRecordsEmitted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.UserXattrRecordsEmitted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.UserXattrRecordsEmitted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1307]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.CompressionPropertyTagged]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.CompressionPropertyTagged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.CompressionPropertyTagged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1382]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.Raid10GeometryGate]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.Raid10GeometryGate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.Raid10GeometryGate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1473]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.Raid1RotatingMirrorReads]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.Raid1RotatingMirrorReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.Raid1RotatingMirrorReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1659]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.Raid0StripeMathSplitsAtStripeBoundaries]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.Raid0StripeMathSplitsAtStripeBoundaries]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.Raid0StripeMathSplitsAtStripeBoundaries]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1689]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Btrfs.DefaultDirItemNonSubvolumeLocationFallsBack]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Btrfs.DefaultDirItemNonSubvolumeLocationFallsBack]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Btrfs.DefaultDirItemNonSubvolumeLocationFallsBack]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_btrfs_volume.cpp:1810]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.EndToEndWalk]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.EndToEndWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.EndToEndWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:904]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.ChildDirZapReadsKernelOffset32]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.ChildDirZapReadsKernelOffset32]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.ChildDirZapReadsKernelOffset32]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:985]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.OriginObjNotTreatedAsChildZap]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.OriginObjNotTreatedAsChildZap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.OriginObjNotTreatedAsChildZap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1009]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.LabelPositionsRecoveredFromTail]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.LabelPositionsRecoveredFromTail]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.LabelPositionsRecoveredFromTail]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1047]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.NewestTxgAcrossLabelsWins]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.NewestTxgAcrossLabelsWins]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.NewestTxgAcrossLabelsWins]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1061]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.ChecksumFailDropsAndRetriesNextDva]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.ChecksumFailDropsAndRetriesNextDva]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.ChecksumFailDropsAndRetriesNextDva]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1099]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.StripedPoolForeignVdevFallbackRead]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.StripedPoolForeignVdevFallbackRead]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.StripedPoolForeignVdevFallbackRead]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1154]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.MirrorSecondMemberScan]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.MirrorSecondMemberScan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.MirrorSecondMemberScan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1225]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.SaRegistryDrivenLayout2]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.SaRegistryDrivenLayout2]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.SaRegistryDrivenLayout2]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1260]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.SaRegistryAbsentFallsBackToLayout1]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.SaRegistryAbsentFallsBackToLayout1]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.SaRegistryAbsentFallsBackToLayout1]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1304]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.BadMagicRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.BadMagicRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.BadMagicRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1345]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.ZleRoundTrip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.ZleRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.ZleRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1355]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.CodecMapping]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.CodecMapping]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.CodecMapping]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1369]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.CodecRoundTrips]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.CodecRoundTrips]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.CodecRoundTrips]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1449]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.TruncatedSourcesRejectedNotOverread]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.TruncatedSourcesRejectedNotOverread]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.TruncatedSourcesRejectedNotOverread]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1477]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Zfs.NonAsciiNamesDecodedNotByteLifted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Zfs.NonAsciiNamesDecodedNotByteLifted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Zfs.NonAsciiNamesDecodedNotByteLifted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_zfs_volume.cpp:1506]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RefsDetect.FindsMsbPages]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RefsDetect.FindsMsbPages]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RefsDetect.FindsMsbPages]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_ldm.cpp:35]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RefsDetect.CheckpointVariant]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RefsDetect.CheckpointVariant]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RefsDetect.CheckpointVariant]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_ldm.cpp:46]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RefsDetect.IdentificationFallback]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RefsDetect.IdentificationFallback]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RefsDetect.IdentificationFallback]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_ldm.cpp:57]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RefsDetect.NonRefsRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RefsDetect.NonRefsRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RefsDetect.NonRefsRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_ldm.cpp:71]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.ParsesDatabaseAndGraph]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.ParsesDatabaseAndGraph]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.ParsesDatabaseAndGraph]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:58]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.ResolvesSimpleVolume]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.ResolvesSimpleVolume]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.ResolvesSimpleVolume]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:94]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.SimpleVolumeDeviceReads]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.SimpleVolumeDeviceReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.SimpleVolumeDeviceReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:114]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.StripedVolumeInterleave]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.StripedVolumeInterleave]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.StripedVolumeInterleave]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:143]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.CompositeVblkRecord]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.CompositeVblkRecord]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.CompositeVblkRecord]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:178]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.NonLdmDiskRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.NonLdmDiskRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.NonLdmDiskRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:196]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.Raid5LeftSymmetricReads]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.Raid5LeftSymmetricReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.Raid5LeftSymmetricReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:266]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.Raid5ReconstructsMissingColumn]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.Raid5ReconstructsMissingColumn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.Raid5ReconstructsMissingColumn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:311]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.Raid5TwoMissingColumnsUnrecoverable]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.Raid5TwoMissingColumnsUnrecoverable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.Raid5TwoMissingColumnsUnrecoverable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:339]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.MirroredReadFailover]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.MirroredReadFailover]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.MirroredReadFailover]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:396]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.MirroredBothCopiesFail]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.MirroredBothCopiesFail]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.MirroredBothCopiesFail]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:423]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.SpannedConcatenation]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.SpannedConcatenation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.SpannedConcatenation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:462]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.GptDynamicDiskCanonicalGuidsParse]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.GptDynamicDiskCanonicalGuidsParse]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.GptDynamicDiskCanonicalGuidsParse]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:597]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.GptDynamicDiskFabricatedGuidsRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.GptDynamicDiskFabricatedGuidsRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.GptDynamicDiskFabricatedGuidsRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:617]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.GptDynamicDiskTableClassifiesLdmEntries]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.GptDynamicDiskTableClassifiesLdmEntries]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.GptDynamicDiskTableClassifiesLdmEntries]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:628]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.GptDynamicDiskRealCanonicalGuidsParse]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.GptDynamicDiskRealCanonicalGuidsParse]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.GptDynamicDiskRealCanonicalGuidsParse]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:670]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.GptPathFindsMetadata4Kn]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.GptPathFindsMetadata4Kn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.GptPathFindsMetadata4Kn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:763]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.TocblockAtConfigPlus0]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.TocblockAtConfigPlus0]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.TocblockAtConfigPlus0]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:806]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.TocblockPrefersPlus2OnConflict]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.TocblockPrefersPlus2OnConflict]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.TocblockPrefersPlus2OnConflict]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:826]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.TocblockTailCopies]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.TocblockTailCopies]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.TocblockTailCopies]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:850]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.OversizeConfigRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.OversizeConfigRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.OversizeConfigRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:868]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.ConfigSizeExactlyOneMebibyteAccepted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.ConfigSizeExactlyOneMebibyteAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.ConfigSizeExactlyOneMebibyteAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:890]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Ldm.ZeroInterleaveDefaultsTo64K]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Ldm.ZeroInterleaveDefaultsTo64K]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Ldm.ZeroInterleaveDefaultsTo64K]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_ldm.cpp:902]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.FindsScoresAndRejectsNoise]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.FindsScoresAndRejectsNoise]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.FindsScoresAndRejectsNoise]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:127]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.OverlapClassification]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.OverlapClassification]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.OverlapClassification]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:183]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.TwoHealthyDeletedPartitionsBothSurface]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.TwoHealthyDeletedPartitionsBothSurface]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.TwoHealthyDeletedPartitionsBothSurface]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:204]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.DamagedBpbFallbackMustNotSwallowPeer]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.DamagedBpbFallbackMustNotSwallowPeer]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.DamagedBpbFallbackMustNotSwallowPeer]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:220]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.Ext4SuperblockAnchorsDeletedPartition]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.Ext4SuperblockAnchorsDeletedPartition]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.Ext4SuperblockAnchorsDeletedPartition]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:322]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.Ext4BlockGroupCopyNotACandidateStart]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.Ext4BlockGroupCopyNotACandidateStart]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.Ext4BlockGroupCopyNotACandidateStart]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:353]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.HfsPlusVolumeHeaderAnchorsDeletedPartition]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.HfsPlusVolumeHeaderAnchorsDeletedPartition]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.HfsPlusVolumeHeaderAnchorsDeletedPartition]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:367]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.BtrfsSuperblockAnchorsDeletedPartition]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.BtrfsSuperblockAnchorsDeletedPartition]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.BtrfsSuperblockAnchorsDeletedPartition]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:383]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.ZfsLabelAnchorsDeletedPartition]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.ZfsLabelAnchorsDeletedPartition]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.ZfsLabelAnchorsDeletedPartition]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:398]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.RefsBootAnchorsDeletedPartition]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.RefsBootAnchorsDeletedPartition]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.RefsBootAnchorsDeletedPartition]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:414]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.SuperblockNoiseRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.SuperblockNoiseRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.SuperblockNoiseRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:433]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.LostBitlockerPartitionDiscovered]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.LostBitlockerPartitionDiscovered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.LostBitlockerPartitionDiscovered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:479]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.LostBitlockerOemNoise]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.LostBitlockerOemNoise]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.LostBitlockerOemNoise]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:519]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.LostBitlockerInsideExtendedRegionStillFound]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.LostBitlockerInsideExtendedRegionStillFound]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.LostBitlockerInsideExtendedRegionStillFound]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:557]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.FatBootWithOgGatesBecomesCandidate]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.FatBootWithOgGatesBecomesCandidate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.FatBootWithOgGatesBecomesCandidate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:713]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.Fat32ExtentUsesDataAreaForContainment]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.Fat32ExtentUsesDataAreaForContainment]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.Fat32ExtentUsesDataAreaForContainment]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:731]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.LostCandidateReportsDataAreaOffset]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.LostCandidateReportsDataAreaOffset]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.LostCandidateReportsDataAreaOffset]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:767]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.FileRichCandidateOutranksCleanBoot]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.FileRichCandidateOutranksCleanBoot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.FileRichCandidateOutranksCleanBoot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:885]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.TableMatchBonus500]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.TableMatchBonus500]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.TableMatchBonus500]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:907]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostDiscovery.OverLargeDuplicateRescoredToFront]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostDiscovery.OverLargeDuplicateRescoredToFront]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostDiscovery.OverLargeDuplicateRescoredToFront]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:936]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostFatGates.Spc256Rejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostFatGates.Spc256Rejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostFatGates.Spc256Rejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:622]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostFatGates.NumFatsZeroRejectedThreeAcceptedByDeepGates]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostFatGates.NumFatsZeroRejectedThreeAcceptedByDeepGates]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostFatGates.NumFatsZeroRejectedThreeAcceptedByDeepGates]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:635]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostFatGates.Fat32NameStringTornYieldsNoCandidate]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostFatGates.Fat32NameStringTornYieldsNoCandidate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostFatGates.Fat32NameStringTornYieldsNoCandidate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:659]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostFatGates.Fat12SmallFatProbe]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostFatGates.Fat12SmallFatProbe]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostFatGates.Fat12SmallFatProbe]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:675]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostFatGates.Fat12ProbeSpfBoundIsExclusiveAt200]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostFatGates.Fat12ProbeSpfBoundIsExclusiveAt200]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostFatGates.Fat12ProbeSpfBoundIsExclusiveAt200]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:695]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostFatGates.Fat32GateStringIsCaseSensitive]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostFatGates.Fat32GateStringIsCaseSensitive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostFatGates.Fat32GateStringIsCaseSensitive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:703]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostScoring.MatchesLogRows]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostScoring.MatchesLogRows]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostScoring.MatchesLogRows]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_lostpart.cpp:862]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Sha256.NistVectors]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Sha256.NistVectors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Sha256.NistVectors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_sha256.cpp:9]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Sha256.StreamedMatchesOneShot]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Sha256.StreamedMatchesOneShot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Sha256.StreamedMatchesOneShot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_sha256.cpp:28]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Sha256.Id128Truncation]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Sha256.Id128Truncation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Sha256.Id128Truncation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_sha256.cpp:43]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.Aes128Fips197KnownAnswer]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=AesXts.Aes128Fips197KnownAnswer]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.Aes128Fips197KnownAnswer]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:161]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.Aes256Fips197KnownAnswer]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=AesXts.Aes256Fips197KnownAnswer]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.Aes256Fips197KnownAnswer]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:174]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.Xts128CrossImplementationVector]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=AesXts.Xts128CrossImplementationVector]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.Xts128CrossImplementationVector]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:190]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.Xts256CrossImplementationVector]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=AesXts.Xts256CrossImplementationVector]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.Xts256CrossImplementationVector]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:208]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.Xts256WholeSectorVector]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=AesXts.Xts256WholeSectorVector]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.Xts256WholeSectorVector]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:229]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.WeakKeyHalvesRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=AesXts.WeakKeyHalvesRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.WeakKeyHalvesRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:266]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.RoundTripVariedLengths]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=AesXts.RoundTripVariedLengths]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.RoundTripVariedLengths]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:279]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[AesXts.DistinctTweaksGiveDistinctCiphertexts]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=AesXts.DistinctTweaksGiveDistinctCiphertexts]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[AesXts.DistinctTweaksGiveDistinctCiphertexts]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:304]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DecryptingDevice.ReadsPlaintextThroughWrapper]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DecryptingDevice.ReadsPlaintextThroughWrapper]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DecryptingDevice.ReadsPlaintextThroughWrapper]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:326]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DecryptingDevice.UnalignedAndCrossSectorReads]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DecryptingDevice.UnalignedAndCrossSectorReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DecryptingDevice.UnalignedAndCrossSectorReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:359]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DecryptingDevice.InnerErrorsPropagateVerbatim]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DecryptingDevice.InnerErrorsPropagateVerbatim]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DecryptingDevice.InnerErrorsPropagateVerbatim]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:405]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DecryptingDevice.BadKeyMaterialCreateFails]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DecryptingDevice.BadKeyMaterialCreateFails]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DecryptingDevice.BadKeyMaterialCreateFails]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:419]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[BitLockerStack.EncryptedVolumeScansOnlyThroughDecryptDevice]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=BitLockerStack.EncryptedVolumeScansOnlyThroughDecryptDevice]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[BitLockerStack.EncryptedVolumeScansOnlyThroughDecryptDevice]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_bitlocker_xts.cpp:433]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.ShippedTableMatchesCorpusShape]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Dict.ShippedTableMatchesCorpusShape]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.ShippedTableMatchesCorpusShape]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:77]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.ShippedTableKeepsCorpusDuplicates]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Dict.ShippedTableKeepsCorpusDuplicates]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.ShippedTableKeepsCorpusDuplicates]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:101]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.ParseDictionaryGrammar]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Dict.ParseDictionaryGrammar]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.ParseDictionaryGrammar]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:131]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.ParseToleratesLfAndEmptyLines]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Dict.ParseToleratesLfAndEmptyLines]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.ParseToleratesLfAndEmptyLines]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:143]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.ParseRoundTripsTheCorpusGrammar]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Dict.ParseRoundTripsTheCorpusGrammar]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.ParseRoundTripsTheCorpusGrammar]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:154]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.ParseRejectsBadGrammar]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Dict.ParseRejectsBadGrammar]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.ParseRejectsBadGrammar]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:169]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.TryStopsAtFirstSuccessInShippedOrder]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Dict.TryStopsAtFirstSuccessInShippedOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.TryStopsAtFirstSuccessInShippedOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:190]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.UserCandidatesFollowShippedDictionary]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Dict.UserCandidatesFollowShippedDictionary]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.UserCandidatesFollowShippedDictionary]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:203]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.AllFailReportsEveryAttempt]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Dict.AllFailReportsEveryAttempt]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.AllFailReportsEveryAttempt]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:218]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.NullAttemptFunctionIsANoOp]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Dict.NullAttemptFunctionIsANoOp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.NullAttemptFunctionIsANoOp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:228]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Dict.AttemptsLoggedThroughSeamWithoutPasswordMaterial]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Dict.AttemptsLoggedThroughSeamWithoutPasswordMaterial]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Dict.AttemptsLoggedThroughSeamWithoutPasswordMaterial]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_dict.cpp:234]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.ResumeDropsStaleCandidateAfterRelayout]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.ResumeDropsStaleCandidateAfterRelayout]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.ResumeDropsStaleCandidateAfterRelayout]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:215]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.ResumeKeepsValidCandidate]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.ResumeKeepsValidCandidate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.ResumeKeepsValidCandidate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:263]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.DiskLayoutRecordRoundTrip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.DiskLayoutRecordRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.DiskLayoutRecordRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:288]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.LayoutChangedFlagged]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.LayoutChangedFlagged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.LayoutChangedFlagged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:327]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.SizeSourceFlagRoundTrip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.SizeSourceFlagRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.SizeSourceFlagRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:383]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.CandidateGeometryRoundTrip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.CandidateGeometryRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.CandidateGeometryRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:415]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ResumeRevalidation.ArbiterStartOffDiskDropsAndUnknownKeeps]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ResumeRevalidation.ArbiterStartOffDiskDropsAndUnknownKeeps]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ResumeRevalidation.ArbiterStartOffDiskDropsAndUnknownKeeps]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_resume_revalidation.cpp:483]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.AsciiPassesThrough]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.AsciiPassesThrough]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.AsciiPassesThrough]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:28]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.LatinAccentDecodes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.LatinAccentDecodes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.LatinAccentDecodes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:34]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.CjkDecodesToCodePoints]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.CjkDecodesToCodePoints]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.CjkDecodesToCodePoints]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:39]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.AstralPlaneBecomesSurrogatePair]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.AstralPlaneBecomesSurrogatePair]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.AstralPlaneBecomesSurrogatePair]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:46]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.OverlongFormsRejectedAsReplacement]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.OverlongFormsRejectedAsReplacement]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.OverlongFormsRejectedAsReplacement]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:53]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.SurrogateHalvesRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.SurrogateHalvesRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.SurrogateHalvesRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:67]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.BeyondUnicodeRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.BeyondUnicodeRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.BeyondUnicodeRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:77]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.InvalidLeadBytesResyncOneByte]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.InvalidLeadBytesResyncOneByte]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.InvalidLeadBytesResyncOneByte]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:86]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.TruncatedTailBecomesReplacement]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.TruncatedTailBecomesReplacement]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.TruncatedTailBecomesReplacement]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:93]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.MaximalSubpartReplacement]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.MaximalSubpartReplacement]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.MaximalSubpartReplacement]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:108]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.EmbeddedNulDecodes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.EmbeddedNulDecodes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.EmbeddedNulDecodes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:131]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.RoundTripThroughNodeUtf8]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.RoundTripThroughNodeUtf8]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.RoundTripThroughNodeUtf8]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:137]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.InvalidByteLosslessMapping]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.InvalidByteLosslessMapping]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.InvalidByteLosslessMapping]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:175]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.LosslessRoundTripRecoversRawBytes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.LosslessRoundTripRecoversRawBytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.LosslessRoundTripRecoversRawBytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:195]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.ZfsDecoderStaysReplacement]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.ZfsDecoderStaysReplacement]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.ZfsDecoderStaysReplacement]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:221]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.WideToUtf8EncodesNonAscii]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.WideToUtf8EncodesNonAscii]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.WideToUtf8EncodesNonAscii]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:232]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.WideToUtf8SurrogatePairAndLoneHalf]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.WideToUtf8SurrogatePairAndLoneHalf]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.WideToUtf8SurrogatePairAndLoneHalf]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:250]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Text.CliFilterUtf8MatchesDecodedNames]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Text.CliFilterUtf8MatchesDecodedNames]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Text.CliFilterUtf8MatchesDecodedNames]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_text.cpp:265]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.GrammarValidationAndSpecificity]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.GrammarValidationAndSpecificity]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.GrammarValidationAndSpecificity]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:230]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.CarvesMidSectorStart]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.CarvesMidSectorStart]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.CarvesMidSectorStart]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:324]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.SecondPassFullyDeduped]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.SecondPassFullyDeduped]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.SecondPassFullyDeduped]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:360]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.VideoPostProcessorHookInvoked]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.VideoPostProcessorHookInvoked]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.VideoPostProcessorHookInvoked]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:431]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.HookDegradesWhenModuleAbsent]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.HookDegradesWhenModuleAbsent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.HookDegradesWhenModuleAbsent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:479]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.RegistryF8Gate]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.RegistryF8Gate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.RegistryF8Gate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:496]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.PstAndRar5Grammar]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.PstAndRar5Grammar]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.PstAndRar5Grammar]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:502]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ExtendedFormatsGrammar]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.ExtendedFormatsGrammar]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ExtendedFormatsGrammar]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:597]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.PdfEofLastWinsAndFrameFloors]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.PdfEofLastWinsAndFrameFloors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.PdfEofLastWinsAndFrameFloors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:805]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.RawSignLayerHeaderOnlyAndSharedMagic]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.RawSignLayerHeaderOnlyAndSharedMagic]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.RawSignLayerHeaderOnlyAndSharedMagic]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:897]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.RawSignTailPairWithinFormat]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.RawSignTailPairWithinFormat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.RawSignTailPairWithinFormat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:951]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.UnreadableWindowNotCountedAsScanned]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.UnreadableWindowNotCountedAsScanned]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.UnreadableWindowNotCountedAsScanned]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1097]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ZipCdAnchorCarvesDestroyedHead]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.ZipCdAnchorCarvesDestroyedHead]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ZipCdAnchorCarvesDestroyedHead]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1126]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ZipIndexlessLfhChainSalvage]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.ZipIndexlessLfhChainSalvage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ZipIndexlessLfhChainSalvage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1149]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ZipSalvageAbsorbsDataSectors]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.ZipSalvageAbsorbsDataSectors]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ZipSalvageAbsorbsDataSectors]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1184]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.Zip64ExtraFieldContinuesChain]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.Zip64ExtraFieldContinuesChain]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.Zip64ExtraFieldContinuesChain]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1221]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ZipCleanArchiveNotInflatedByZeroSlack]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.ZipCleanArchiveNotInflatedByZeroSlack]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ZipCleanArchiveNotInflatedByZeroSlack]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1276]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.BeTiffAndOrfVariantsCarve]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.BeTiffAndOrfVariantsCarve]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.BeTiffAndOrfVariantsCarve]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1301]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.OverlapScorePrefersHigherConfidence]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.OverlapScorePrefersHigherConfidence]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.OverlapScorePrefersHigherConfidence]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1372]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.TiffVsRawTiebreak]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.TiffVsRawTiebreak]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.TiffVsRawTiebreak]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1428]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.PstUnknownVersionAcceptedAtFloor]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.PstUnknownVersionAcceptedAtFloor]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.PstUnknownVersionAcceptedAtFloor]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1480]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.JpegSof0PrecisionGate]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.JpegSof0PrecisionGate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.JpegSof0PrecisionGate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1510]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.SlicedDeviceSecondPassFullyDedupedAndAbsolute]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.SlicedDeviceSecondPassFullyDedupedAndAbsolute]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.SlicedDeviceSecondPassFullyDedupedAndAbsolute]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1549]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.CarvedNodesAreDeleted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.CarvedNodesAreDeleted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.CarvedNodesAreDeleted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2081]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.RawSignCarvedNodesAreDeleted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.RawSignCarvedNodesAreDeleted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.RawSignCarvedNodesAreDeleted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2118]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.DeletedOnlyQueryIncludesCarvedFiles]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.DeletedOnlyQueryIncludesCarvedFiles]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.DeletedOnlyQueryIncludesCarvedFiles]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2143]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.CarvedNodesCarryScanTimestamp]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.CarvedNodesCarryScanTimestamp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.CarvedNodesCarryScanTimestamp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2177]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.RawSignCarvedNodesCarryScanTimestamp]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.RawSignCarvedNodesCarryScanTimestamp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.RawSignCarvedNodesCarryScanTimestamp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2214]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ScanTimestampSurvivesSessionRoundTrip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.ScanTimestampSurvivesSessionRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ScanTimestampSurvivesSessionRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2245]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ZipInnerEntryInventoryEmitted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.ZipInnerEntryInventoryEmitted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ZipInnerEntryInventoryEmitted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2449]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ZipIndexlessEntryInventory]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.ZipIndexlessEntryInventory]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ZipIndexlessEntryInventory]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2486]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.Rar4InnerEntryInventory]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.Rar4InnerEntryInventory]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.Rar4InnerEntryInventory]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2533]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ProvenanceChecksumAndAnchorsPersist]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.ProvenanceChecksumAndAnchorsPersist]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ProvenanceChecksumAndAnchorsPersist]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3051]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.TailAnchorCarriedOnTailPairedHits]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.TailAnchorCarriedOnTailPairedHits]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.TailAnchorCarriedOnTailPairedHits]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3106]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.BitmapSkipsUsedClusters]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.BitmapSkipsUsedClusters]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.BitmapSkipsUsedClusters]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3144]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.NoBitmapCarvesEverything]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.NoBitmapCarvesEverything]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.NoBitmapCarvesEverything]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3188]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.FormatDisabledSkipsSignature]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.FormatDisabledSkipsSignature]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.FormatDisabledSkipsSignature]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3235]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.FormatDisabledRawSignsToo]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.FormatDisabledRawSignsToo]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.FormatDisabledRawSignsToo]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3271]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.AllFormatsEnabledByDefault]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.AllFormatsEnabledByDefault]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.AllFormatsEnabledByDefault]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3305]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.WindowServingEliminatesPerCandidateRereads]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.WindowServingEliminatesPerCandidateRereads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.WindowServingEliminatesPerCandidateRereads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3418]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.WindowServingServesStraddlingReadsCorrectly]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.WindowServingServesStraddlingReadsCorrectly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.WindowServingServesStraddlingReadsCorrectly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3450]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.Mp4NamedByMetadataTemplate]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.Mp4NamedByMetadataTemplate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.Mp4NamedByMetadataTemplate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3495]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.MovNamedByMetadataTemplate]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.MovNamedByMetadataTemplate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.MovNamedByMetadataTemplate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3518]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.MovV1MvhdHighWordQuirkReplicated]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.MovV1MvhdHighWordQuirkReplicated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.MovV1MvhdHighWordQuirkReplicated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3538]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.FallbackNamerDocumented]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.FallbackNamerDocumented]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.FallbackNamerDocumented]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3561]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.ZeroSectorSkipStats]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.ZeroSectorSkipStats]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.ZeroSectorSkipStats]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3596]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.WindowBufferReusedByValidatorChecksumAndHash]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.WindowBufferReusedByValidatorChecksumAndHash]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.WindowBufferReusedByValidatorChecksumAndHash]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3657]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[CarveEngine.BoundaryStraddlingCandidateSplitsReadsCorrectly]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=CarveEngine.BoundaryStraddlingCandidateSplitsReadsCorrectly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[CarveEngine.BoundaryStraddlingCandidateSplitsReadsCorrectly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3687]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawSigns.UnitsAreKib]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawSigns.UnitsAreKib]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawSigns.UnitsAreKib]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:876]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawSigns.CarvesFiveMiBVob]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawSigns.CarvesFiveMiBVob]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawSigns.CarvesFiveMiBVob]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:983]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawSigns.MinFloorOneKib]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawSigns.MinFloorOneKib]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawSigns.MinFloorOneKib]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1012]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawSigns.TailPairWindowScaled]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawSigns.TailPairWindowScaled]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawSigns.TailPairWindowScaled]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1033]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.Mp4RealFtypIsom]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.Mp4RealFtypIsom]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.Mp4RealFtypIsom]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1638]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MovRealFtypQt]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.MovRealFtypQt]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MovRealFtypQt]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1659]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.HeicRealFtypHeic]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.HeicRealFtypHeic]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.HeicRealFtypHeic]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1675]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.M4aRealFtyp]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.M4aRealFtyp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.M4aRealFtyp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1691]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MovMoovWithoutFtyp]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.MovMoovWithoutFtyp]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MovMoovWithoutFtyp]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1707]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.RegistrySelfProbeReachesEveryFormat]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.RegistrySelfProbeReachesEveryFormat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.RegistrySelfProbeReachesEveryFormat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1732]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.S007CommonFormatsCarve]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.S007CommonFormatsCarve]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.S007CommonFormatsCarve]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1914]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.S007ValidatorsRejectNoise]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.S007ValidatorsRejectNoise]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.S007ValidatorsRejectNoise]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:1971]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.S007TruncatedSalvageSemantics]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.S007TruncatedSalvageSemantics]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.S007TruncatedSalvageSemantics]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2033]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.ZipTruncatedHeadRecovered]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.ZipTruncatedHeadRecovered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.ZipTruncatedHeadRecovered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2345]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.ZipLfhSanityGates]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.ZipLfhSanityGates]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.ZipLfhSanityGates]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2374]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MkvDocTypeWebmAccepted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.MkvDocTypeWebmAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MkvDocTypeWebmAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2663]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MkvDocTypeExoticRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.MkvDocTypeExoticRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MkvDocTypeExoticRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2681]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.MkvElementBoundsGate]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.MkvElementBoundsGate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.MkvElementBoundsGate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2697]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.HeifSiblingBrandLabeledHeif]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.HeifSiblingBrandLabeledHeif]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.HeifSiblingBrandLabeledHeif]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2718]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.IiuTiffEpValidatedByTiffGrammar]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.IiuTiffEpValidatedByTiffGrammar]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.IiuTiffEpValidatedByTiffGrammar]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2743]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.IiuRw2FallbackWhenTiffEpRejects]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.IiuRw2FallbackWhenTiffEpRejects]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.IiuRw2FallbackWhenTiffEpRejects]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2765]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.RafVariantHeaderAccepted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.RafVariantHeaderAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.RafVariantHeaderAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2784]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.RafRealAsciiVersionAccepted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.RafRealAsciiVersionAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.RafRealAsciiVersionAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2810]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.JpegFourthByteOutsideOgSet]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.JpegFourthByteOutsideOgSet]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.JpegFourthByteOutsideOgSet]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2838]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.JpegTruncatedTailSalvagedAtSofEstimate]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.JpegTruncatedTailSalvagedAtSofEstimate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.JpegTruncatedTailSalvagedAtSofEstimate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2876]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.JpegTruncatedTailWithoutSofSalvagesAtCapTenth]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.JpegTruncatedTailWithoutSofSalvagesAtCapTenth]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.JpegTruncatedTailWithoutSofSalvagesAtCapTenth]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2911]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.JpegCompleteFileStopsAtEoiNotSofEstimate]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.JpegCompleteFileStopsAtEoiNotSofEstimate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.JpegCompleteFileStopsAtEoiNotSofEstimate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:2948]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Carve.CabLowercaseMagic]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Carve.CabLowercaseMagic]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Carve.CabLowercaseMagic]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_carve.cpp:3012]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.RoundTrip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Crsf.RoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.RoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:54]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.CorruptionSkipsForward]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Crsf.CorruptionSkipsForward]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.CorruptionSkipsForward]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:98]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.TreeRoundTrip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Crsf.TreeRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.TreeRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:126]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.TinyChunkDeflatesWhenSmaller]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Crsf.TinyChunkDeflatesWhenSmaller]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.TinyChunkDeflatesWhenSmaller]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:185]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.VolumeInfoIdentityCardRoundTrip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Crsf.VolumeInfoIdentityCardRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.VolumeInfoIdentityCardRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:210]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Crsf.TornCheckpointSalvagesPrefix]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Crsf.TornCheckpointSalvagesPrefix]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Crsf.TornCheckpointSalvagesPrefix]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:256]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SessionTree.RefreshStatsIsRecursive]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=SessionTree.RefreshStatsIsRecursive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SessionTree.RefreshStatsIsRecursive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:359]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SessionTree.ReloadedRootStatsAreRecursive]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=SessionTree.ReloadedRootStatsAreRecursive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SessionTree.ReloadedRootStatsAreRecursive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:373]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SessionTree.StatsDeletedCountRecursive]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=SessionTree.StatsDeletedCountRecursive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SessionTree.StatsDeletedCountRecursive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:402]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.GradeRoleAndProvenanceRoundTrip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Session.GradeRoleAndProvenanceRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.GradeRoleAndProvenanceRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:507]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.OldFormatStillLoads]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Session.OldFormatStillLoads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.OldFormatStillLoads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:552]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.ForwardParentDeferredAndCounted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Session.ForwardParentDeferredAndCounted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.ForwardParentDeferredAndCounted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:576]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.BadMarkerFailsTheRecord]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Session.BadMarkerFailsTheRecord]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.BadMarkerFailsTheRecord]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:631]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.TruncatedTailFailsAfterFullCount]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Session.TruncatedTailFailsAfterFullCount]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.TruncatedTailFailsAfterFullCount]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:641]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.HostileNameAndRoleLengthBombs]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Session.HostileNameAndRoleLengthBombs]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.HostileNameAndRoleLengthBombs]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:654]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.CorruptRecordEarlyExitsReleaseProps]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Session.CorruptRecordEarlyExitsReleaseProps]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.CorruptRecordEarlyExitsReleaseProps]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:715]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.RawNameRoundTrip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Session.RawNameRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.RawNameRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:809]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.OldFormatWithoutRawNameStillLoads]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Session.OldFormatWithoutRawNameStillLoads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.OldFormatWithoutRawNameStillLoads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:847]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.SaveEmitsProgress]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Session.SaveEmitsProgress]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.SaveEmitsProgress]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:879]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.SaveCancelsCleanly]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Session.SaveCancelsCleanly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.SaveCancelsCleanly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:923]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.AutoNamingConvention]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Session.AutoNamingConvention]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.AutoNamingConvention]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:948]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.BtrfsExtrasRoundTrip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Session.BtrfsExtrasRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.BtrfsExtrasRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:978]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.ZfsSecondaryFlagsRoundTrip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Session.ZfsSecondaryFlagsRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.ZfsSecondaryFlagsRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1013]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Session.EngineExtrasAbsentWhenUnpopulated]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Session.EngineExtrasAbsentWhenUnpopulated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Session.EngineExtrasAbsentWhenUnpopulated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_session.cpp:1054]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.DynamicDiskVolumeScan]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.DynamicDiskVolumeScan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.DynamicDiskVolumeScan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:171]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.JunkFilteredFromResults]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.JunkFilteredFromResults]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.JunkFilteredFromResults]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:266]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.LogSilentByDefault]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.LogSilentByDefault]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.LogSilentByDefault]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:403]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.LogSinkReceivesScanBrackets]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.LogSinkReceivesScanBrackets]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.LogSinkReceivesScanBrackets]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:423]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.MemoryMonitorEmitsUnderPressure]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.MemoryMonitorEmitsUnderPressure]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.MemoryMonitorEmitsUnderPressure]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:467]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.WeightedProgress]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.WeightedProgress]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.WeightedProgress]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:498]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.ProbeFamily]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.ProbeFamily]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.ProbeFamily]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:508]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.SpoofedExfatOemRejectedBeforeEngine]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.SpoofedExfatOemRejectedBeforeEngine]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.SpoofedExfatOemRejectedBeforeEngine]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:539]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.SpoofedFatBootUnreadableRootRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.SpoofedFatBootUnreadableRootRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.SpoofedFatBootUnreadableRootRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:552]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.RealExfatBootContentVerified]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.RealExfatBootContentVerified]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.RealExfatBootContentVerified]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:565]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.FsFamilyValuesMatchContract]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.FsFamilyValuesMatchContract]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.FsFamilyValuesMatchContract]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:625]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.UdfProbeFailsOnMkudffsShapeLikeTheOg]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.UdfProbeFailsOnMkudffsShapeLikeTheOg]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.UdfProbeFailsOnMkudffsShapeLikeTheOg]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:642]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.ProbesUdfVolume]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.ProbesUdfVolume]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.ProbesUdfVolume]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:690]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.UdfProbeRequiresBothDescriptorReads]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.UdfProbeRequiresBothDescriptorReads]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.UdfProbeRequiresBothDescriptorReads]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:698]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.UdfNotConfusedWithFat]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.UdfNotConfusedWithFat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.UdfNotConfusedWithFat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:712]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.UdfSurfacesAsCarveFamily]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.UdfSurfacesAsCarveFamily]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.UdfSurfacesAsCarveFamily]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:736]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.CarveSingleNamingFamily]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.CarveSingleNamingFamily]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.CarveSingleNamingFamily]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:858]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.StagedRunAndSessionRoundTrip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.StagedRunAndSessionRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.StagedRunAndSessionRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:982]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.PreScanRepairRestoresTornFatAndScans]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.PreScanRepairRestoresTornFatAndScans]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.PreScanRepairRestoresTornFatAndScans]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1263]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.PreScanRepairDeniedNeverWrites]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.PreScanRepairDeniedNeverWrites]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.PreScanRepairDeniedNeverWrites]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1315]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.PreScanRepairWithoutHooksNeverWrites]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.PreScanRepairWithoutHooksNeverWrites]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.PreScanRepairWithoutHooksNeverWrites]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1347]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.ReadPipeBuildsCacheOverRetryForValidGeometry]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.ReadPipeBuildsCacheOverRetryForValidGeometry]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.ReadPipeBuildsCacheOverRetryForValidGeometry]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1698]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.ReadPipeDegradesToBareSliceWhenCacheCreationFails]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.ReadPipeDegradesToBareSliceWhenCacheCreationFails]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.ReadPipeDegradesToBareSliceWhenCacheCreationFails]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1713]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Orchestrator.ReadPipeZeroCapacityDegradesToBareSlice]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Orchestrator.ReadPipeZeroCapacityDegradesToBareSlice]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Orchestrator.ReadPipeZeroCapacityDegradesToBareSlice]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1730]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FilterFolder.CorpusListsPinned]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FilterFolder.CorpusListsPinned]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FilterFolder.CorpusListsPinned]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:219]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SafeWriter.OverlapRefusal]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=SafeWriter.OverlapRefusal]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SafeWriter.OverlapRefusal]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1065]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SafeWriter.CollisionAndSanitize]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=SafeWriter.CollisionAndSanitize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SafeWriter.CollisionAndSanitize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1111]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SafeWriter.ResidentPrefixNotOverwritten]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=SafeWriter.ResidentPrefixNotOverwritten]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SafeWriter.ResidentPrefixNotOverwritten]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1127]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[SafeWriter.InvalidUtf8NameRestoredFromRaw]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=SafeWriter.InvalidUtf8NameRestoredFromRaw]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[SafeWriter.InvalidUtf8NameRestoredFromRaw]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1648]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanEvents.ErrorRangeGoesToLiveSink]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ScanEvents.ErrorRangeGoesToLiveSink]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanEvents.ErrorRangeGoesToLiveSink]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1432]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanEvents.SinkSwitchDeliversAllTypes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ScanEvents.SinkSwitchDeliversAllTypes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanEvents.SinkSwitchDeliversAllTypes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1460]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanEvents.SingleBoundaryAtTeardown]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ScanEvents.SingleBoundaryAtTeardown]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanEvents.SingleBoundaryAtTeardown]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1489]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanEvents.ErrorRangeCarriesModuleOperationAttempts]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ScanEvents.ErrorRangeCarriesModuleOperationAttempts]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanEvents.ErrorRangeCarriesModuleOperationAttempts]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1558]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanEvents.ProbeBlockFailureAttributesToOrchestrator]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ScanEvents.ProbeBlockFailureAttributesToOrchestrator]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanEvents.ProbeBlockFailureAttributesToOrchestrator]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1582]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanEvents.CarveErrorRangeAttributesToCarveModule]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ScanEvents.CarveErrorRangeAttributesToCarveModule]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanEvents.CarveErrorRangeAttributesToCarveModule]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1606]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ScanConfig.DeepStageDefaultsOnAndWholeDiskOff]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ScanConfig.DeepStageDefaultsOnAndWholeDiskOff]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ScanConfig.DeepStageDefaultsOnAndWholeDiskOff]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_orchestrator.cpp:1628]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.TokenCheckpointBlocksAndResumes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PauseResume.TokenCheckpointBlocksAndResumes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.TokenCheckpointBlocksAndResumes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:186]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.CancelWhilePausedWakesWaiter]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PauseResume.CancelWhilePausedWakesWaiter]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.CancelWhilePausedWakesWaiter]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:209]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.PausedMsAccounting]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PauseResume.PausedMsAccounting]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.PausedMsAccounting]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:225]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.ScanPauseFreezesAndElapsedExcludesPause]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PauseResume.ScanPauseFreezesAndElapsedExcludesPause]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.ScanPauseFreezesAndElapsedExcludesPause]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:240]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.SaveOnCancelStampsTruthfulMaskAndResumeSkipsQuick]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PauseResume.SaveOnCancelStampsTruthfulMaskAndResumeSkipsQuick]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.SaveOnCancelStampsTruthfulMaskAndResumeSkipsQuick]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:278]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.MaskIntegrityViolationRefusesResume]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PauseResume.MaskIntegrityViolationRefusesResume]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.MaskIntegrityViolationRefusesResume]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:364]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.ResumedSessionPreservesOriginalConfiguredMask]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PauseResume.ResumedSessionPreservesOriginalConfiguredMask]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.ResumedSessionPreservesOriginalConfiguredMask]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:435]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.SessionRestoresVolumeInfoAndCandidates]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PauseResume.SessionRestoresVolumeInfoAndCandidates]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.SessionRestoresVolumeInfoAndCandidates]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:512]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PauseResume.SaveAfterResumeReloadsCleanly]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PauseResume.SaveAfterResumeReloadsCleanly]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PauseResume.SaveAfterResumeReloadsCleanly]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:868]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Checkpoint.StageBoundaryCheckpointResumesAfterCrash]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Checkpoint.StageBoundaryCheckpointResumesAfterCrash]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Checkpoint.StageBoundaryCheckpointResumesAfterCrash]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:605]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Checkpoint.CorruptMagicRefused]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Checkpoint.CorruptMagicRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Checkpoint.CorruptMagicRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:671]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Checkpoint.CommitHookFiresAfterEveryFinalizeAndAbortsOnError]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Checkpoint.CommitHookFiresAfterEveryFinalizeAndAbortsOnError]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Checkpoint.CommitHookFiresAfterEveryFinalizeAndAbortsOnError]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:951]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExtentBasis.TreeRebaseShiftsOnceAndIsIdempotent]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExtentBasis.TreeRebaseShiftsOnceAndIsIdempotent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExtentBasis.TreeRebaseShiftsOnceAndIsIdempotent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:723]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExtentBasis.SliceScanSavesAbsoluteExtents]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExtentBasis.SliceScanSavesAbsoluteExtents]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExtentBasis.SliceScanSavesAbsoluteExtents]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:753]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[ExtentBasis.LegacySliceRelativeSessionNormalizedOnLoad]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=ExtentBasis.LegacySliceRelativeSessionNormalizedOnLoad]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[ExtentBasis.LegacySliceRelativeSessionNormalizedOnLoad]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pause_resume.cpp:806]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Telemetry.UniformProgressAcrossStagesMonotonic]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Telemetry.UniformProgressAcrossStagesMonotonic]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Telemetry.UniformProgressAcrossStagesMonotonic]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_telemetry.cpp:209]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Telemetry.FatWalkEmitsIncrementalTicks]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Telemetry.FatWalkEmitsIncrementalTicks]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Telemetry.FatWalkEmitsIncrementalTicks]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_telemetry.cpp:244]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Telemetry.PerStageShapeAndCompletion]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Telemetry.PerStageShapeAndCompletion]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Telemetry.PerStageShapeAndCompletion]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_telemetry.cpp:289]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Telemetry.LiveErrorMapVisibleMidScan]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Telemetry.LiveErrorMapVisibleMidScan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Telemetry.LiveErrorMapVisibleMidScan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_telemetry.cpp:316]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Telemetry.QueryDuringScanIsThreadSafe]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Telemetry.QueryDuringScanIsThreadSafe]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Telemetry.QueryDuringScanIsThreadSafe]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_telemetry.cpp:364]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.StructureReconstructionWithEmptyLeaves]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=WriterDirs.StructureReconstructionWithEmptyLeaves]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.StructureReconstructionWithEmptyLeaves]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:203]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.DeepDestChainCreatedAndIdempotentRerun]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=WriterDirs.DeepDestChainCreatedAndIdempotentRerun]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.DeepDestChainCreatedAndIdempotentRerun]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:238]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.IntraQueueDuplicatesRenameUnderSkip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=WriterDirs.IntraQueueDuplicatesRenameUnderSkip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.IntraQueueDuplicatesRenameUnderSkip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:286]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.PerItemErrorCollectedQueueContinues]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=WriterDirs.PerItemErrorCollectedQueueContinues]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.PerItemErrorCollectedQueueContinues]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:348]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.MetadataOnlyItemSkipsNotZeroFills]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=WriterDirs.MetadataOnlyItemSkipsNotZeroFills]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.MetadataOnlyItemSkipsNotZeroFills]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:387]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.ProgressExcludesErrorItems]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=WriterDirs.ProgressExcludesErrorItems]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.ProgressExcludesErrorItems]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:450]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.CollisionNameUnderscoreFormat]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=WriterDirs.CollisionNameUnderscoreFormat]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.CollisionNameUnderscoreFormat]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:489]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.CollisionExhaustionFailsTheItem]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=WriterDirs.CollisionExhaustionFailsTheItem]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.CollisionExhaustionFailsTheItem]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:542]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[WriterDirs.MkdirSemanticsMatchShCreateDirectoryEx]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=WriterDirs.MkdirSemanticsMatchShCreateDirectoryEx]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[WriterDirs.MkdirSemanticsMatchShCreateDirectoryEx]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_writer_dirs.cpp:585]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Reghive.ParsesValidHiveAndRejectsGarbage]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Reghive.ParsesValidHiveAndRejectsGarbage]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Reghive.ParsesValidHiveAndRejectsGarbage]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:209]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Reghive.RejectsCorruptedHeaderChecksum]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Reghive.RejectsCorruptedHeaderChecksum]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Reghive.RejectsCorruptedHeaderChecksum]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:223]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Reghive.WalksControlSetKeyFamilyCaseInsensitive]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Reghive.WalksControlSetKeyFamilyCaseInsensitive]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Reghive.WalksControlSetKeyFamilyCaseInsensitive]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:232]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Reghive.EnumeratesSubkeyNames]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Reghive.EnumeratesSubkeyNames]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Reghive.EnumeratesSubkeyNames]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:244]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Reghive.ReadsBinaryValueDataPointedAndInline]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Reghive.ReadsBinaryValueDataPointedAndInline]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Reghive.ReadsBinaryValueDataPointedAndInline]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:258]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Reghive.UnicodeKeyNamesWalkAsAscii]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Reghive.UnicodeKeyNamesWalkAsAscii]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Reghive.UnicodeKeyNamesWalkAsAscii]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:294]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Reghive.TruncatedAndMutatedHivesParseOrRefuseWithoutCrashing]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Reghive.TruncatedAndMutatedHivesParseOrRefuseWithoutCrashing]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Reghive.TruncatedAndMutatedHivesParseOrRefuseWithoutCrashing]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:409]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FveAutoUnlock.HarvestsPerGuidDataBlobs]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FveAutoUnlock.HarvestsPerGuidDataBlobs]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FveAutoUnlock.HarvestsPerGuidDataBlobs]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:310]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FveAutoUnlock.HarvestCapsBlobsAt256Bytes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FveAutoUnlock.HarvestCapsBlobsAt256Bytes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FveAutoUnlock.HarvestCapsBlobsAt256Bytes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:331]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FveAutoUnlock.HarvestCapsRecordsAt100AndSkipsTinyBlobs]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FveAutoUnlock.HarvestCapsRecordsAt100AndSkipsTinyBlobs]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FveAutoUnlock.HarvestCapsRecordsAt100AndSkipsTinyBlobs]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:349]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[FveAutoUnlock.SelectionPrefersMatchingVolumeGuid]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=FveAutoUnlock.SelectionPrefersMatchingVolumeGuid]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FveAutoUnlock.SelectionPrefersMatchingVolumeGuid]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_reghive.cpp:371]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pwddict.ParsesOgeDictFormatByteFaithfully]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pwddict.ParsesOgeDictFormatByteFaithfully]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pwddict.ParsesOgeDictFormatByteFaithfully]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pwddict.cpp:79]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pwddict.ParseAcceptsLfAndOptionalComma]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pwddict.ParseAcceptsLfAndOptionalComma]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pwddict.ParseAcceptsLfAndOptionalComma]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pwddict.cpp:99]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pwddict.ParseEnforcesCapsAndRefusesOversize]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pwddict.ParseEnforcesCapsAndRefusesOversize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pwddict.ParseEnforcesCapsAndRefusesOversize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pwddict.cpp:109]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pwddict.SeedDictionaryIsSane]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pwddict.SeedDictionaryIsSane]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pwddict.SeedDictionaryIsSane]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pwddict.cpp:143]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pwddict.TryPasswordsStopsAtFirstSuccess]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pwddict.TryPasswordsStopsAtFirstSuccess]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pwddict.TryPasswordsStopsAtFirstSuccess]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pwddict.cpp:172]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pwddict.TryPasswordsDedupsPreservingOrder]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pwddict.TryPasswordsDedupsPreservingOrder]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pwddict.TryPasswordsDedupsPreservingOrder]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pwddict.cpp:184]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pwddict.TryPasswordsExhaustsToLockedAndGuardsArgs]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pwddict.TryPasswordsExhaustsToLockedAndGuardsArgs]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pwddict.TryPasswordsExhaustsToLockedAndGuardsArgs]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pwddict.cpp:198]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pwddict.AttemptLogCarriesProgressAndLengthNeverSecrets]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pwddict.AttemptLogCarriesProgressAndLengthNeverSecrets]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pwddict.AttemptLogCarriesProgressAndLengthNeverSecrets]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pwddict.cpp:221]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Pwddict.ExhaustLogsWarnOnce]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Pwddict.ExhaustLogsWarnOnce]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Pwddict.ExhaustLogsWarnOnce]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_pwddict.cpp:243]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Preview.ExtentAccurateReadsWithSparseTailAndEof]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Preview.ExtentAccurateReadsWithSparseTailAndEof]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Preview.ExtentAccurateReadsWithSparseTailAndEof]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:97]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Preview.BoundedReadCannotBeTricked]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Preview.BoundedReadCannotBeTricked]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Preview.BoundedReadCannotBeTricked]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:127]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Preview.ReadFailuresZeroFilledAndReported]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Preview.ReadFailuresZeroFilledAndReported]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Preview.ReadFailuresZeroFilledAndReported]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:149]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Preview.CompressedExtentDecodes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Preview.CompressedExtentDecodes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Preview.CompressedExtentDecodes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:166]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Preview.ConcurrentIndependentReaders]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Preview.ConcurrentIndependentReaders]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Preview.ConcurrentIndependentReaders]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:198]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Preview.MalformedCapsExtentKeepsAlignment]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Preview.MalformedCapsExtentKeepsAlignment]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Preview.MalformedCapsExtentKeepsAlignment]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:927]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.JpegDimensionsAndExif]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PreviewMeta.JpegDimensionsAndExif]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.JpegDimensionsAndExif]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:228]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.PngGifBmpDimensions]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PreviewMeta.PngGifBmpDimensions]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.PngGifBmpDimensions]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:290]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.WavDurationAndMp4Brand]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PreviewMeta.WavDurationAndMp4Brand]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.WavDurationAndMp4Brand]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:346]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.PdfTextAndPlainAndUnsupported]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PreviewMeta.PdfTextAndPlainAndUnsupported]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.PdfTextAndPlainAndUnsupported]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:403]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.TruncatedMagicsAreSafeAndHonest]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PreviewMeta.TruncatedMagicsAreSafeAndHonest]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.TruncatedMagicsAreSafeAndHonest]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:481]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.WavFmtChunkOutsideHeadCache]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PreviewMeta.WavFmtChunkOutsideHeadCache]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.WavFmtChunkOutsideHeadCache]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:528]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.DocxRawDeflateInflatesAndPlainZipStaysZip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PreviewMeta.DocxRawDeflateInflatesAndPlainZipStaysZip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.DocxRawDeflateInflatesAndPlainZipStaysZip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:559]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.DocxCapsMaxReadHonored]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PreviewMeta.DocxCapsMaxReadHonored]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.DocxCapsMaxReadHonored]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:790]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.DocxTailBeyondBudgetDegradesToZip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PreviewMeta.DocxTailBeyondBudgetDegradesToZip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.DocxTailBeyondBudgetDegradesToZip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:815]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.CompressedExtentReportsTruncatedUnderBudget]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PreviewMeta.CompressedExtentReportsTruncatedUnderBudget]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.CompressedExtentReportsTruncatedUnderBudget]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:834]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.CompressedExtentWholeBlockReadDisclosed]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PreviewMeta.CompressedExtentWholeBlockReadDisclosed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.CompressedExtentWholeBlockReadDisclosed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:868]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.MetadataTruncatedFlagTruthfulAllPaths]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PreviewMeta.MetadataTruncatedFlagTruthfulAllPaths]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.MetadataTruncatedFlagTruthfulAllPaths]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:906]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[PreviewMeta.RandomSmallInputsAreSafe]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=PreviewMeta.RandomSmallInputsAreSafe]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[PreviewMeta.RandomSmallInputsAreSafe]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_preview.cpp:950]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Snapshot.CaptureFidelityAndDfsParity]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Snapshot.CaptureFidelityAndDfsParity]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Snapshot.CaptureFidelityAndDfsParity]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_snapshot.cpp:112]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Snapshot.PartialFlagAndEmptyTree]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Snapshot.PartialFlagAndEmptyTree]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Snapshot.PartialFlagAndEmptyTree]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_snapshot.cpp:182]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Snapshot.ConcurrentEmitCaptureWalk]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Snapshot.ConcurrentEmitCaptureWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Snapshot.ConcurrentEmitCaptureWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_snapshot.cpp:193]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Snapshot.RecoveryFromSnapshotIsByteIdentical]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Snapshot.RecoveryFromSnapshotIsByteIdentical]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Snapshot.RecoveryFromSnapshotIsByteIdentical]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_snapshot.cpp:243]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.CategoryMap]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Query.CategoryMap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.CategoryMap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:85]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.EmailCategoryExists]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Query.EmailCategoryExists]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.EmailCategoryExists]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:97]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.BookmarksCategory]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Query.BookmarksCategory]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.BookmarksCategory]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:107]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.CameraRawClassified]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Query.CameraRawClassified]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.CameraRawClassified]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:111]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.TableCountMatchesCorpusUniques]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Query.TableCountMatchesCorpusUniques]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.TableCountMatchesCorpusUniques]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:125]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.CrossListedFirstWriterWins]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Query.CrossListedFirstWriterWins]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.CrossListedFirstWriterWins]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:131]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.FilterSortPageCorrectness]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Query.FilterSortPageCorrectness]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.FilterSortPageCorrectness]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:143]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.PathPrefixFilterMatchesAndPrunes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Query.PathPrefixFilterMatchesAndPrunes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.PathPrefixFilterMatchesAndPrunes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:265]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.RandomizedAgainstReference]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Query.RandomizedAgainstReference]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.RandomizedAgainstReference]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:342]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.TypeSortKeyOrdersByCategoryThenExtension]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Query.TypeSortKeyOrdersByCategoryThenExtension]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.TypeSortKeyOrdersByCategoryThenExtension]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:375]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.ModifiedUnitIsFiletimeEverywhere]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Query.ModifiedUnitIsFiletimeEverywhere]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.ModifiedUnitIsFiletimeEverywhere]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:421]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.DateBucketsTranslateToRanges]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Query.DateBucketsTranslateToRanges]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.DateBucketsTranslateToRanges]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:542]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.SizeBucketsTranslateToRanges]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Query.SizeBucketsTranslateToRanges]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.SizeBucketsTranslateToRanges]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:607]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Query.BucketFiltersThroughRun]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Query.BucketFiltersThroughRun]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Query.BucketFiltersThroughRun]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:667]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Error.ErrTextCoversEveryCode]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Error.ErrTextCoversEveryCode]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Error.ErrTextCoversEveryCode]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query.cpp:453]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[QueryPerf.MillionNodesFilterCountUnder200ms]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=QueryPerf.MillionNodesFilterCountUnder200ms]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[QueryPerf.MillionNodesFilterCountUnder200ms]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_query_perf.cpp:34]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DiskMap.RangeFreeEdgeMatrix]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DiskMap.RangeFreeEdgeMatrix]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DiskMap.RangeFreeEdgeMatrix]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_diskmap.cpp:39]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DiskMap.FreeMapRecordRoundTripAndRejection]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DiskMap.FreeMapRecordRoundTripAndRejection]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DiskMap.FreeMapRecordRoundTripAndRejection]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_diskmap.cpp:71]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DiskMap.FreeOverlayHonorsZoomWindow]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DiskMap.FreeOverlayHonorsZoomWindow]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DiskMap.FreeOverlayHonorsZoomWindow]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_diskmap.cpp:109]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DiskMap.DownsamplePreservesRawTruth]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DiskMap.DownsamplePreservesRawTruth]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DiskMap.DownsamplePreservesRawTruth]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_diskmap.cpp:136]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DiskMap.FreeNeverMarksFoundContentCells]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DiskMap.FreeNeverMarksFoundContentCells]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DiskMap.FreeNeverMarksFoundContentCells]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_diskmap.cpp:163]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[DiskMap.OverlayBudgetGrowsCellSize]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=DiskMap.OverlayBudgetGrowsCellSize]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[DiskMap.OverlayBudgetGrowsCellSize]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_diskmap.cpp:203]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.PlansEntryIntoFirstFreeSlot]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Apply.PlansEntryIntoFirstFreeSlot]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.PlansEntryIntoFirstFreeSlot]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:164]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.ExecuteWritesEntryVerifiesAndUndoes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Apply.ExecuteWritesEntryVerifiesAndUndoes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.ExecuteWritesEntryVerifiesAndUndoes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:184]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.RefusedWithoutConsent]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Apply.RefusedWithoutConsent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.RefusedWithoutConsent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:216]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.StaleTableRefusedBeforeConsent]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Apply.StaleTableRefusedBeforeConsent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.StaleTableRefusedBeforeConsent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:236]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.QuiesceFailureIsACleanRefusal]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Apply.QuiesceFailureIsACleanRefusal]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.QuiesceFailureIsACleanRefusal]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:259]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.WriteFailureRollsBackEverything]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Apply.WriteFailureRollsBackEverything]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.WriteFailureRollsBackEverything]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:274]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.ContainmentRefusals]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Apply.ContainmentRefusals]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.ContainmentRefusals]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:299]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.SignatureRestorePlannedWhenAbsent]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Apply.SignatureRestorePlannedWhenAbsent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.SignatureRestorePlannedWhenAbsent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:360]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.ChsClampsBeyondCylinder1023]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Apply.ChsClampsBeyondCylinder1023]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.ChsClampsBeyondCylinder1023]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:385]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Apply.FamilyTypeMap]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Apply.FamilyTypeMap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Apply.FamilyTypeMap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:411]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.WritesEntryAndRefreshesModernPath]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostApply.WritesEntryAndRefreshesModernPath]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.WritesEntryAndRefreshesModernPath]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:568]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.LegacyPathMatchesCorpusSequence]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostApply.LegacyPathMatchesCorpusSequence]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.LegacyPathMatchesCorpusSequence]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:610]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.GptRebuildRegeneratesSignature]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostApply.GptRebuildRegeneratesSignature]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.GptRebuildRegeneratesSignature]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:644]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.ConsentBeforeLockDenialHoldsNothing]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostApply.ConsentBeforeLockDenialHoldsNothing]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.ConsentBeforeLockDenialHoldsNothing]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:684]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.UndoApplyRestoresAndRefreshes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostApply.UndoApplyRestoresAndRefreshes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.UndoApplyRestoresAndRefreshes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:707]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.RefreshFailureIsReportedNotSwallowed]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostApply.RefreshFailureIsReportedNotSwallowed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.RefreshFailureIsReportedNotSwallowed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:730]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.RollsBackWhenVerifyFails]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostApply.RollsBackWhenVerifyFails]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.RollsBackWhenVerifyFails]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:754]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[LostApply.PlanIsReadOnlyAndRefusalsAreResults]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=LostApply.PlanIsReadOnlyAndRefusalsAreResults]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[LostApply.PlanIsReadOnlyAndRefusalsAreResults]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_apply.cpp:772]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.CleanExfatNeverTouched]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.CleanExfatNeverTouched]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.CleanExfatNeverTouched]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:154]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExfatChecksumRepair]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.ExfatChecksumRepair]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExfatChecksumRepair]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:177]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExfatDestroyedPrimaryRestoredFromBackup]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.ExfatDestroyedPrimaryRestoredFromBackup]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExfatDestroyedPrimaryRestoredFromBackup]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:212]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.NtfsBackupRestore]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.NtfsBackupRestore]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.NtfsBackupRestore]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:250]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.NtfsCleanRefused]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.NtfsCleanRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.NtfsCleanRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:286]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.NtfsNoBackupRefused]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.NtfsNoBackupRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.NtfsNoBackupRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:306]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.Fat32BackupRestore]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.Fat32BackupRestore]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.Fat32BackupRestore]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:319]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExfatGeometryRederivation]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.ExfatGeometryRederivation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExfatGeometryRederivation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:357]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExfatAmbiguousGeometryRefused]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.ExfatAmbiguousGeometryRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExfatAmbiguousGeometryRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:383]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.UnknownFamilyRefused]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.UnknownFamilyRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.UnknownFamilyRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:406]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExecuteRefusesEmptyPlan]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.ExecuteRefusesEmptyPlan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExecuteRefusesEmptyPlan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:416]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FailedWriteRollsBackEverything]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.FailedWriteRollsBackEverything]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FailedWriteRollsBackEverything]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:424]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.UndoLogRoundTrip]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.UndoLogRoundTrip]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.UndoLogRoundTrip]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:458]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.MftRecord0RestoredFromMirror]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.MftRecord0RestoredFromMirror]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.MftRecord0RestoredFromMirror]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:609]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.MftCleanNeverTouched]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.MftCleanNeverTouched]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.MftCleanNeverTouched]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:634]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.MftBothInvalidNoWrite]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.MftBothInvalidNoWrite]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.MftBothInvalidNoWrite]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:644]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.MftRestoreMakesVolumeOpenable]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.MftRestoreMakesVolumeOpenable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.MftRestoreMakesVolumeOpenable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:659]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FatTornMirrorCopyRestoredSectorGranular]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.FatTornMirrorCopyRestoredSectorGranular]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FatTornMirrorCopyRestoredSectorGranular]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:690]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FatTornPrimaryRestoredFromMirror]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.FatTornPrimaryRestoredFromMirror]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FatTornPrimaryRestoredFromMirror]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:723]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FatMirrorAmbiguityNeverWritten]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.FatMirrorAmbiguityNeverWritten]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FatMirrorAmbiguityNeverWritten]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:740]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FatCleanMirrorNoAction]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.FatCleanMirrorNoAction]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FatCleanMirrorNoAction]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:757]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FixByScanSectorRebuildsBootFromOnlyCandidate]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.FixByScanSectorRebuildsBootFromOnlyCandidate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FixByScanSectorRebuildsBootFromOnlyCandidate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:766]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FixByScanSectorAmbiguousCandidatesRefused]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.FixByScanSectorAmbiguousCandidatesRefused]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FixByScanSectorAmbiguousCandidatesRefused]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:793]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.FixByScanSectorIdenticalCandidatesNotAmbiguous]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.FixByScanSectorIdenticalCandidatesNotAmbiguous]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.FixByScanSectorIdenticalCandidatesNotAmbiguous]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:808]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.RebasedPartitionPlanWritesAtDiskBasisNeverLba0]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.RebasedPartitionPlanWritesAtDiskBasisNeverLba0]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.RebasedPartitionPlanWritesAtDiskBasisNeverLba0]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:827]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.VolumeLockAcquiredBeforeFirstWrite]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.VolumeLockAcquiredBeforeFirstWrite]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.VolumeLockAcquiredBeforeFirstWrite]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:881]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.VolumeLockFailureNeverWrites]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.VolumeLockFailureNeverWrites]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.VolumeLockFailureNeverWrites]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:900]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.UnlockHappensOnRollbackPath]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.UnlockHappensOnRollbackPath]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.UnlockHappensOnRollbackPath]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:920]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.NtfsPrimaryDamaged4Kn]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.NtfsPrimaryDamaged4Kn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.NtfsPrimaryDamaged4Kn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:992]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExfatViaBackup4Kn]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.ExfatViaBackup4Kn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExfatViaBackup4Kn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:1024]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.AnalyzeAllBranches4KnAsan]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.AnalyzeAllBranches4KnAsan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.AnalyzeAllBranches4KnAsan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:1057]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExfatRestorePlanIsSectorSized4Kn]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.ExfatRestorePlanIsSectorSized4Kn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExfatRestorePlanIsSectorSized4Kn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:1108]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[RawFixer.ExfatBackupReadFull4Kn]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=RawFixer.ExfatBackupReadFull4Kn]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[RawFixer.ExfatBackupReadFull4Kn]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_rawfixer.cpp:1143]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Consistency.ExtensionIsLastDotUppercased]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Consistency.ExtensionIsLastDotUppercased]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Consistency.ExtensionIsLastDotUppercased]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:61]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Consistency.CheckableVocabularyIsSignatureFamiliesPlusSpecials]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Consistency.CheckableVocabularyIsSignatureFamiliesPlusSpecials]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Consistency.CheckableVocabularyIsSignatureFamiliesPlusSpecials]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:69]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Consistency.ClassifierIsSubstringPlusVideoSpecials]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Consistency.ClassifierIsSubstringPlusVideoSpecials]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Consistency.ClassifierIsSubstringPlusVideoSpecials]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:90]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.ContentInconsistentGhostRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Lost.ContentInconsistentGhostRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.ContentInconsistentGhostRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:132]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.ContentConsistentGhostAccepted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Lost.ContentConsistentGhostAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.ContentConsistentGhostAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:156]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.RatioBoundaryThirtyPercent]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Lost.RatioBoundaryThirtyPercent]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.RatioBoundaryThirtyPercent]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:174]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.MajorityGateRequiresHalfFilterable]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Lost.MajorityGateRequiresHalfFilterable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.MajorityGateRequiresHalfFilterable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:199]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.DirectoriesAndExtentlessFilesMirrorOgCounters]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Lost.DirectoriesAndExtentlessFilesMirrorOgCounters]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.DirectoriesAndExtentlessFilesMirrorOgCounters]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:219]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.DataStartOutOfBoundsInvalidatesTheCandidate]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Lost.DataStartOutOfBoundsInvalidatesTheCandidate]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.DataStartOutOfBoundsInvalidatesTheCandidate]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:247]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.UnreadableDataStartStaysUnmatched]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Lost.UnreadableDataStartStaysUnmatched]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.UnreadableDataStartStaysUnmatched]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:259]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.EmptyCandidateListNeverRejects]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Lost.EmptyCandidateListNeverRejects]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.EmptyCandidateListNeverRejects]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:283]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.WholeDiskScanDropsContentInconsistentExfatGhost]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Lost.WholeDiskScanDropsContentInconsistentExfatGhost]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.WholeDiskScanDropsContentInconsistentExfatGhost]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:542]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Lost.WholeDiskScanKeepsContentConsistentExfatGhost]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Lost.WholeDiskScanKeepsContentConsistentExfatGhost]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Lost.WholeDiskScanKeepsContentConsistentExfatGhost]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:591]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[TreeDetach.RemovesNodesRootsAndIndexes]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=TreeDetach.RemovesNodesRootsAndIndexes]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TreeDetach.RemovesNodesRootsAndIndexes]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:308]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[TreeDetach.HardLinkPropsSurviveWithTheirRefs]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=TreeDetach.HardLinkPropsSurviveWithTheirRefs]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[TreeDetach.HardLinkPropsSurviveWithTheirRefs]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_consistency.cpp:347]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.BootValidation]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.BootValidation]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.BootValidation]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:502]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.FullScanHierarchy]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.FullScanHierarchy]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.FullScanHierarchy]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:517]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.FallbackRootSkipsOgSystemFamilies]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.FallbackRootSkipsOgSystemFamilies]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.FallbackRootSkipsOgSystemFamilies]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:568]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.DeletedGradingAndId2]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.DeletedGradingAndId2]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.DeletedGradingAndId2]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:624]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.OrchestratorProbeAndScan]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.OrchestratorProbeAndScan]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.OrchestratorProbeAndScan]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:660]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.NextChainFindsNewerTable]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.NextChainFindsNewerTable]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.NextChainFindsNewerTable]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:675]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.CheckpointLayoutMinor14]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.CheckpointLayoutMinor14]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.CheckpointLayoutMinor14]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:763]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.Pre34CheckpointRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.Pre34CheckpointRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.Pre34CheckpointRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:804]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.CheckpointBodyMinorGatesLayoutNotVbr]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.CheckpointBodyMinorGatesLayoutNotVbr]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.CheckpointBodyMinorGatesLayoutNotVbr]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:819]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.VbrMinor14BodyMinorOldListStillOldLayout]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.VbrMinor14BodyMinorOldListStillOldLayout]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.VbrMinor14BodyMinorOldListStillOldLayout]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:861]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.ResidentInlineFileRecovered]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.ResidentInlineFileRecovered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.ResidentInlineFileRecovered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:951]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.ResidentDollarIMetadataRead]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.ResidentDollarIMetadataRead]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.ResidentDollarIMetadataRead]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1000]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.MultipleDataAttributesMerged]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.MultipleDataAttributesMerged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.MultipleDataAttributesMerged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1042]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.PropertyFlagAndUnknownRowsDoNotBreakDataWalk]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.PropertyFlagAndUnknownRowsDoNotBreakDataWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.PropertyFlagAndUnknownRowsDoNotBreakDataWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1076]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.PagedRunTableFollowed]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.PagedRunTableFollowed]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.PagedRunTableFollowed]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1146]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.Gen7OldGrammarDatarunAttribute]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.Gen7OldGrammarDatarunAttribute]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.Gen7OldGrammarDatarunAttribute]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1194]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.RunlistGranularityMismatchDiscarded]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.RunlistGranularityMismatchDiscarded]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.RunlistGranularityMismatchDiscarded]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1228]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.StaleMsbPageDeletedFileRecovered]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.StaleMsbPageDeletedFileRecovered]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.StaleMsbPageDeletedFileRecovered]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1260]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.StaleSweepSkipsLivePages]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.StaleSweepSkipsLivePages]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.StaleSweepSkipsLivePages]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1297]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.HardLinkRowsCollapseToOneResult]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.HardLinkRowsCollapseToOneResult]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.HardLinkRowsCollapseToOneResult]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1323]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.RecycleRoutedFlaggedAndDollarIRemap]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.RecycleRoutedFlaggedAndDollarIRemap]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.RecycleRoutedFlaggedAndDollarIRemap]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1354]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.ProgressMonotonicIntermediateTicks]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.ProgressMonotonicIntermediateTicks]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.ProgressMonotonicIntermediateTicks]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1451]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.ProgressCancellationMidWalk]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.ProgressCancellationMidWalk]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.ProgressCancellationMidWalk]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1493]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.Bps256Accepted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.Bps256Accepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.Bps256Accepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1539]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.Bps1024Accepted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.Bps1024Accepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.Bps1024Accepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1553]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.Cluster16KRejected]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.Cluster16KRejected]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.Cluster16KRejected]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1565]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.Cluster64KAccepted]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.Cluster64KAccepted]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.Cluster64KAccepted]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1576]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.OpenReadIsFourKiB]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.OpenReadIsFourKiB]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.OpenReadIsFourKiB]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1588]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.ContainerRelativeRunTranslatedPerOgFormula]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.ContainerRelativeRunTranslatedPerOgFormula]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.ContainerRelativeRunTranslatedPerOgFormula]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1635]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.PhysicalRunFlag20Untranslated]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.PhysicalRunFlag20Untranslated]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.PhysicalRunFlag20Untranslated]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1665]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.ContainerBlockBelowSpanStaysPhysical]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.ContainerBlockBelowSpanStaysPhysical]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.ContainerBlockBelowSpanStaysPhysical]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1691]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
add_test([=[Refs.SingleContainerExtentsUnchanged]=]  /home/z/my-project/work/core_recovery/build-linux/tests/cr_tests [==[--gtest_filter=Refs.SingleContainerExtentsUnchanged]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[Refs.SingleContainerExtentsUnchanged]=]
  PROPERTIES
    
    DEF_SOURCE_LINE [==[/home/z/my-project/work/core_recovery/tests/unit/test_refs_volume.cpp:1717]==]
    WORKING_DIRECTORY [==[/home/z/my-project/work/core_recovery/build-linux/tests]==]
    SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==]
    
)
set(cr_tests_TESTS [==[Fixup.ValidRecordApplies]==] [==[Fixup.TornRecordRejected]==] [==[Fixup.TornThirdTailStillSalvaged]==] [==[Fixup.MinSizeGate]==] [==[Fixup.ZeroOffsetRejected]==] [==[Fixup.FourKnSectorSize]==] [==[Image.FlatFileScansMbr]==] [==[Image.FlatRejectsEmptyAndMissing]==] [==[Image.ImageRejectsTruncatedFile]==] [==[Image.EdiskRefusedAsRegisteredExclusion]==] [==[Image.BadMagicVmdkRefusedNotFlatFallback]==] [==[Image.VmdkSparseExtentReads]==] [==[Image.VmdkDifferencingChainReadsParent]==] [==[Image.VmdkStreamOptimizedRefused]==] [==[Image.VhdDynamicDisk]==] [==[Image.VhdDifferencingReadsParentOnClearBitmap]==] [==[Image.VhdBadChecksumRefused]==] [==[Image.VhdxDynamicReads]==] [==[Image.VhdxLogOverlayServesUnflushedWrites]==] [==[Image.VhdxDifferencingRefusedPrecisely]==] [==[Image.Lba0LayoutClassification]==] [==[Image.ExtensionClassificationMatchesOgDispatch]==] [==[Image.SessionRoundTripPersistsImagePath]==] [==[Image.QemuVmdkCompat]==] [==[Image.QemuVhdCompat]==] [==[Image.QemuVhdxCompat]==] [==[NvmeHealth.PageFieldOffsets]==] [==[NvmeHealth.FullFieldSurface]==] [==[NvmeHealth.DataUnitsConvertedToBytes]==] [==[NvmeHealth.ShortBufferRejected]==] [==[NvmeHealth.NullRejected]==] [==[NvmeHealth.PercentageUsedIsU8Width]==] [==[Nvme.CliTempFractionIsCelsiusDecidegrees]==] [==[Health.SmartCommandListMatchesImplementation]==] [==[Health.Attr197Raw3IsCaution]==] [==[Health.ValueBelowVendorThresholdIsBad]==] [==[Health.Attr5RawAllFFSkipsCautionCheck]==] [==[Health.DuplicateAttrIdYieldsUnknown]==] [==[Health.NoGradedSignalYieldsUnknown]==] [==[Health.SsdAnyAttrBelowThresholdIsBad]==] [==[Health.TemperatureNeverAffectsTheDiskGrade]==] [==[Health.SsdLifeFamilyGatesAndBoundaries]==] [==[Health.SsdLifeBelowVendorThresholdIsBad]==] [==[Health.SsdLifeInvertFormReads100MinusRaw]==] [==[Health.WdcAttr230LifeFromRawByte]==] [==[Health.Attr1RuleIsSandForceGated]==] [==[Health.FailingBitForcesDiskBadButAttrGradesFollowOg]==] [==[Health.NvmeGradeMapping]==] [==[Health.ModelSaysSsdOcList]==] [==[Health.ClassifyFamilyDecodedVendors]==] [==[Health.ClassifyFamilyAttributeSignatures]==] [==[Health.ClassifyFamilyFormsAndFallbacks]==] [==[Health.MakeGradeInputsAppliesOgDefaults]==] [==[Health.GradeNames]==] [==[Health.CsmiRequestMatchesOgWireFormat]==] [==[Health.CsmiIdentifyCarriesZeroLbaAndSmartCarriesMagic]==] [==[Health.CsmiBuilderRejectsUndersizedBuffer]==] [==[Health.CsmiResponseDataExtractedAtOgOffset]==] [==[Health.CsmiTransportUnavailableClassifiesTheOgErrorSet]==] [==[Health.CsmiTransportFallsBack]==] [==[Health.CsmiReadsAttributes]==] [==[Health.SmartChainEmptyIsBadParameter]==] [==[Health.SmartAttrNameTableCoversTheCuratedIds]==] [==[RunList.SingleRunDecode]==] [==[RunList.NegativeDelta]==] [==[RunList.SparseRun]==] [==[RunList.TerminatesAndRejectsTruncated]==] [==[RunList.ZeroLengthRunRejected]==] [==[RunList.MaxClusterGuard]==] [==[RunList.EncodeDecodeRoundTrip]==] [==[RunList.SignedWidthEdgeCases]==] [==[RunList.ExtentCoalesce]==] [==[RunList.Int64MinOffsetRejected]==] [==[Crc32.KnownVectors]==] [==[Gpt.ValidTable]==] [==[Gpt.BadHeaderCrcDetected]==] [==[Gpt.DeadPrimaryHeaderSalvagesOwnEntries]==] [==[Gpt.ProtectiveOnlyVolumeRecoversFromBackup]==] [==[Gpt.HostileSalvageHeaderEntriesOutsideDiskRejected]==] [==[Gpt.CanonicalTypeGuidConstantsMatchRealDiskBytes]==] [==[Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText]==] [==[Gpt.EspMsrLinuxBiosbootLdmClassification]==] [==[Mbr.PlainWithExtended]==] [==[Mbr.EbrLayoutAssignedAndPlainMbrStaysMbr]==] [==[Mbr.EbrCycleGuard]==] [==[Mbr.TornLinkKeepsPriorVolumes]==] [==[Mbr.TornLinkStatusReported]==] [==[Slice.BoundsEnforced]==] [==[Slice.TailReadRefusalPinned]==] [==[Slice.StartMustBeAligned]==] [==[Slice.RaggedTailAllowed]==] [==[Slice.GeometryRefreshAfterMediaChange]==] [==[Slice.GeometryRefreshRefusesInvalidatingChange]==] [==[DevMap.AddMergeSerialize]==] [==[DevMap.ModuleAttributionRecordedAndNewestWinsOnCoalesce]==] [==[DevMap.ModuleNotSerializedWireStaysTrio]==] [==[Cached.HitMissAndBridging]==] [==[Cached.BoundsReject]==] [==[Cached.FailedBlocksAreNotCachedForever]==] [==[Cached.ScanCacheBudgetIsTheRegisteredDivergence]==] [==[DirectAlign.AlignedRequestIsTheFastPath]==] [==[DirectAlign.FourKMultipleNotSixtyFourKTakesTheEnvelope]==] [==[DirectAlign.UnalignedOffsetAndLengthEnvelope]==] [==[DirectAlign.MultiBlockEnvelopeCoversTheRequest]==] [==[DirectAlign.DeviceTailClampIsTheDocumentedException]==] [==[DirectAlign.OneMiBWriterChunkAtAlignedOffsetStaysDirect]==] [==[DirectAlign.OutOfBoundsAndEmptyRequestsFail]==] [==[DirectAlign.IssuedLengthAlwaysSixtyFourKMultipleExceptTail]==] [==[DirectAlign.UnifiedEnvelopeExhaustivePropertyBothDevices]==] [==[Retry.CleanReadPassesThrough]==] [==[Retry.FailModeReportsExhaustedAndMapsRange]==] [==[Retry.SkipModeZeroFillsOnlyBadSectors]==] [==[Retry.UnalignedBadRangeRoundsToWholeSectors]==] [==[Retry.BacktrackReadSalvagesPartialSector]==] [==[Retry.BacktrackOffReproducesLegacyExhausted]==] [==[Retry.BacktrackStaysAlignedOn4Kn]==] [==[Retry.BoundsAreEnforcedBeforeInner]==] [==[Retry.DeterministicFailureNotRetried]==] [==[Retry.GeometryForwarded]==] [==[Retry.ThreadStampAttributedToRecordedRange]==] [==[Retry.OwnerFallbackCoversUnstampedWorkerThreads]==] [==[Errmod.GuardSetsAndRestoresTheThreadStamp]==] [==[Pool.RunsAllItemsExactlyOnce]==] [==[Pool.AbortStopsWork]==] [==[Pool.ExternalCancelTokenAdapter]==] [==[Pool.ExceptionsNeverCrossThreadEdge]==] [==[Pool.WaitIdleAndPending]==] [==[Pool.CancelPoolWatcherDrainsQueueWithinOneTick]==] [==[Pool.CancelPoolWatcherNullTokenIsInert]==] [==[ParallelCarve.MatchesSingleThreaded]==] [==[ParallelRemnants.SameRecordsAsSequential]==] [==[NodeTree.AddRootsAndFiles]==] [==[NodeTree.HardLinkSharesProps]==] [==[NodeTree.ExtentDedupIndex]==] [==[NodeTree.ResetFreesClean]==] [==[Grading.MonotonicMaxNeverDowngrades]==] [==[Grading.Grade2Sticky]==] [==[Grading.DefaultGradeIsAverageAtEmit]==] [==[Grading.SizeFloorIs1]==] [==[Grading.EmailCategoryFastPath]==] [==[Grading.ExistingFilesHighDeletedNot]==] [==[Grading.SmallFileUnder1024High]==] [==[Grading.NtfsDeviceIndexRuleUpgrades]==] [==[Grading.ExtensionTableGatesDeepValidation]==] [==[Grading.TwinAddressFlagsBoth]==] [==[Grading.TableMembershipAndCount]==] [==[Grading.RegradeOnlyGrade1]==] [==[Grading.AssistantCountersMixedFixture]==] [==[Grading.SessionRoundTripPreservesGrade]==] [==[Report.ContainsAssistantStatistics]==] [==[Report.EscapingStrictJson]==] [==[Report.SchemaShapeAndDeterminism]==] [==[Report.WriterOutcomesAndHashHonesty]==] [==[Report.MultiExtentSpannedVolumeRefused]==] [==[Report.DestFullFastFailStopsAttempting]==] [==[Report.DestFullPreCheckRefusesUpfront]==] [==[Report.SalvagedOutcomeMarksZeroFilledCommits]==] [==[Report.EmitsCiSchemaSample]==] [==[Enumerate.ScopeDeltaRegisteredWithRevisitTrigger]==] [==[Enumerate.BusyDiskMetadataRetryShipsInQueryDisk]==] [==[Enumerate.ScopeContractDocumentedInHeader]==] [==[Enumerate.SetupApiMerge]==] [==[Enumerate.DedupByInstancePath]==] [==[Enumerate.MergeWithEmptyInterfaceStackKeepsLoop]==] [==[Enumerate.MergeWithEmptyLoopTakesAllInterfaces]==] [==[NtfsVolume.NonresidentAttrlistOnMft]==] [==[NtfsVolume.NestedAttrlistChase]==] [==[NtfsVolume.ResidentAdsCarriesContent]==] [==[NtfsVolume.AdsAttachesToFirstLink]==] [==[NtfsVolume.UsnJrnlHarvest]==] [==[NtfsVolume.UsnJrnlSparseV3Resync]==] [==[NtfsVolume.ShiftedHeaderSalvage]==] [==[NtfsVolume.EndToEndWalk]==] [==[NtfsVolume.HardLinkSingleLinkMatchesOg]==] [==[NtfsVolume.HardLinkFanOutOptIn]==] [==[NtfsVolume.RecycleBinMapping]==] [==[NtfsVolume.RemnantScanFindsDetachedRecord]==] [==[NtfsVolume.ViabilityProbeUsesDataSize]==] [==[NtfsVolume.ViabilityBoundaryExactFastPath]==] [==[NtfsVolume.ExtensionRecordsNotCountedDamaged]==] [==[NtfsVolume.CompressedDataRecoversRealContent]==] [==[NtfsVolume.Lznt1MsXcaDocumentedVector]==] [==[NtfsVolume.RemnantScanReconstructsDataRunlist]==] [==[NtfsVolume.RemnantScanReconstructsResidentData]==] [==[NtfsVolume.RemnantScanCompressedStreamStaysMetadataOnly]==] [==[NtfsVolume.OverwrittenHonestModeKeepsAndGrades]==] [==[NtfsVolume.HardLinkDestroyRuleInteraction]==] [==[NtfsVolume.BootNoPow2RecordSizeAcceptedAtDecodedValue]==] [==[NtfsVolume.BootRecordSizeZeroRejected]==] [==[NtfsVolume.BootNegativeExponentOverflowRejected]==] [==[NtfsVolume.BootUsableLargeRecordSizeKept]==] [==[NtfsVolume.RemnantReachableUnderTornDbr]==] [==[NtfsVolume.MirrorCoversRunlistSize]==] [==[NtfsVolume.MirrorResidentDataRejected]==] [==[NtfsVolume.UserFileNamedDollarNotHidden]==] [==[NtfsVolume.MainWalkKeepsOverwrittenDeleted]==] [==[NtfsVolume.GuidDumpPassRecoversReusedRecord]==] [==[NtfsVolume.GuidDumpDestroyRules]==] [==[NtfsVolume.SizeZeroRescueFromMftCopy]==] [==[NtfsVolume.SizeZeroRescueSameNameMerge]==] [==[NtfsVolume.GuidDumpStrideIsRecordAligned]==] [==[NtfsVolume.GuidDumpLargeRecordParsedFully]==] [==[NtfsVolume.MainWalkKeepsSystemFlaggedUserFile]==] [==[NtfsVolume.OrphanIndxBlockHarvestsEntries]==] [==[NtfsVolume.IndxHiddenSystemEntryRejected]==] [==[NtfsVolume.IndxSlackNeverHarvested]==] [==[NtfsVolume.RemnantDirRecordHarvestsIndexRootChildren]==] [==[NtfsVolume.AdsDroppedInParityMode]==] [==[NtfsVolume.AdsEmittedByDefaultSuperset]==] [==[NtfsVolume.AttributesFromSIWhenAvailable]==] [==[NtfsVolume.AttributesFallBackToFileNameWhenSIAbsent]==] [==[NtfsVolume.AttributesPreserveOfflineBit]==] [==[NtfsVolume.ShortNameFieldRoundtrips]==] [==[NtfsVolume.UsnHarvestTrueDeleteReason0x80020000]==] [==[NtfsVolume.UsnHarvestRejectsCompressionChangeOnly]==] [==[NtfsVolume.UsnHarvestCapMinesTail]==] [==[NtfsVolume.DiskMapFreeCellsFromRealBitmap]==] [==[NtfsVolume.MultiHopAttrlistOgParityStopsAtDepth1]==] [==[NtfsVolume.DeletedAttrlistChaseFollowsChildrenAtDepth1]==] [==[NtfsVolume.TornPrimaryBackupBootScans]==] [==[NtfsVolume.TornPrimaryShapeBackupBootScans]==] [==[NtfsVolume.BackupBadFallsBackToRefusal]==] [==[NtfsVolume.BootFromBackupFlagged]==] [==[NtfsVolume.SpoofedOemRejectedEarly]==] [==[NtfsVolume.RecycleBinScopeOutOfBinDollarFilesStayRaw]==] [==[FatEngine.FreeMapExcludesVirtualClustersAndMatchesFAT]==] [==[FatEngine.Fat16EndToEnd]==] [==[FatEngine.LfnCorruptFallsBackToShort]==] [==[FatEngine.Fat32RootChainAndBackupBoot]==] [==[FatEngine.Fat16HasNoBackupBootFallback]==] [==[FatEngine.Fat32BackupBootFallback]==] [==[FatEngine.ChainCycleIsCut]==] [==[FatEngine.Fat12ChainsCanonicalEocAndBad]==] [==[FatEngine.DeletedLfnFragmentOrder]==] [==[FatEngine.DeletedLfnRealWorldChecksumRecoversFullName]==] [==[FatEngine.DeletedLfnSingleSlotChecksumRecovery]==] [==[FatEngine.DeletedLfnTornChecksumKeepsHonestFallback]==] [==[FatEngine.LfnDeletedChecksumBijection]==] [==[FatEngine.DeletedUnlinkedChainGetsContiguousRun]==] [==[FatEngine.DeletedUnlinkedChainTruncatesAtReusedCluster]==] [==[FatEngine.DeletedLinkedChainStillPrefersFatTruth]==] [==[FatEngine.Fat32EocMask24AndAlternateFamiliesEndClean]==] [==[FatEngine.Fat16BadClusterStillCanonical]==] [==[FatEngine.LargeFat32TableAccepted]==] [==[FatEngine.SmallFat16TableDeclined]==] [==[FatEngine.MirrorShiftedCopyRejected]==] [==[FatEngine.MirrorCrossLinkRejected]==] [==[FatEngine.MirrorOnlyScanFat1Unreadable]==] [==[FatEngine.CrossLinkedFat1RejectedAndFat2Used]==] [==[FatEngine.MediaByteCanonicalEnforcedFat16]==] [==[FatEngine.InvalidDateYieldsZeroFileTime]==] [==[FatEngine.InvalidMonthYieldsZeroFileTime]==] [==[FatEngine.InvalidDayYieldsZeroFileTime]==] [==[FatEngine.ValidDateYieldsCorrectFileTime]==] [==[FatEngine.FlavorBoundary65524IsFat32]==] [==[FatEngine.BootStringGateDeclinesTornStringVolume]==] [==[FatEngine.BootStringCaseInsensitiveAccepted]==] [==[FatEngine.Fat16LowRootEntriesDeclined]==] [==[FatEngine.Fat16RootEntriesNotSectorMultipleDeclined]==] [==[FatEngine.NumFatsThreeDeclined]==] [==[FatEngine.SpcDomainMatchesOgSet]==] [==[FatEngine.FatBps1536Accepted]==] [==[FatEngine.FatBps32768CeilingAndNonMultipleRejected]==] [==[FatEngine.FatZeroNumFatsRejected]==] [==[FatEngine.FatClusterCountOverflowRejected]==] [==[FatEngine.DateSanityRejectedInStrictMode]==] [==[FatEngine.BadPrimaryServedWithoutMirrorConsult]==] [==[FatEngine.PrimaryFreeStillConsultsMirror]==] [==[FatEngine.RecyclePairingRestoresOriginalName]==] [==[FatEngine.RecycleLegacyRoots]==] [==[FatEngine.NonRecycleDollarFilesStayRaw]==] [==[ExfatEngine.TimestampsMatchOgLocalAsUtc]==] [==[ExfatEngine.TimestampsZeroTzPassThrough]==] [==[ExfatEngine.TimestampsMatchExplorerDisplay]==] [==[ExfatEngine.TimestampsWestOffsetAlsoIgnored]==] [==[ExfatEngine.FreeMapExcludesVirtualClustersAndMatchesBitmap]==] [==[ExfatEngine.TornBitmapReadYieldsNoFreeMap]==] [==[ExfatEngine.EndToEnd]==] [==[ExfatEngine.UpcaseTableChecksumValidated]==] [==[ExfatEngine.UpcaseLengthSanity]==] [==[ExfatEngine.FreeMapProducerMatchesSpecBitMapping]==] [==[ExfatEngine.SetChecksumMismatchGradesExcellentInOgProfile]==] [==[ExfatEngine.SetChecksumMismatchMarksDamaged]==] [==[ExfatEngine.InvalidFirstClusterRejected]==] [==[ExfatEngine.EocFamilyTerminatesChain]==] [==[ExfatEngine.FreeEntryContiguousSalvage]==] [==[ExfatEngine.BootGatesMatchOg]==] [==[ExfatEngine.UpcaseAlgorithmMatchesRealMkfsExfat]==] [==[ExfatEngine.LargeFatFullChains]==] [==[ExfatEngine.LargeFatDirectoryWalk]==] [==[ExfatEngine.StreamReservedPartiallyNonzeroPassesOgByteOr]==] [==[ExfatEngine.StreamReservedAllBytesNonzeroStillDamaged]==] [==[ExfatEngine.StreamReserved1ByteStrictlyZero]==] [==[ExfatEngine.DeletedSetChecksumRejectsTornSet]==] [==[ExfatEngine.DeletedSetOracleReassembly]==] [==[ExfatEngine.LiveCompleteSetBadChecksumUngraded]==] [==[ExfatEngine.EmptyFileSuppressedLikeOg]==] [==[ExfatEngine.GarbageTimestampRejected]==] [==[ExfatEngine.SecondaryCountCapped]==] [==[ExfatEngine.BadNameFlagsRejected]==] [==[ExfatEngine.FileReservedNonzeroRejected]==] [==[ExfatEngine.ValidBoundaryTimestampsStillPass]==] [==[ExfatEngine.NameUtf16TableInvalidCharRejected]==] [==[ExfatEngine.NameUtf16TableAllowedCharAccepted]==] [==[ExfatEngine.RecyclePairingMirrored]==] [==[Ext4Engine.ExtentTreeDepth3]==] [==[Ext4Engine.SuperblockValidation]==] [==[Ext4Engine.EndToEndWalk]==] [==[Ext4Engine.BadMagicRejected]==] [==[Ext4Engine.UnixTimeConversion]==] [==[Ext4Engine.JournalSalvage]==] [==[Ext4Engine.InodeSizeOffsetIsSpecTrue]==] [==[Ext4Engine.JournalUncommittedTailSurfaces]==] [==[Ext4Engine.JournalUnreferencedDirBlockSalvaged]==] [==[Ext4Engine.JournalFirstBlockNotJsbNoSalvage]==] [==[Ext4Engine.CorruptRecLenSkipsEntryNotBlock]==] [==[Ext4Engine.InodeZeroHoleContinues]==] [==[Ext4Engine.FileTypeGateSkipsInvalid]==] [==[Ext4Engine.NonAsciiNamesDecodedNotByteLifted]==] [==[Ext4Engine.MetaBgVolumeRecoversAllGroups]==] [==[Ext4Engine.MetaBgDeletedSweepReachesDistributedGroups]==] [==[Ext4Engine.MetaBgDescriptorBlockReadOnce]==] [==[Ext4Engine.MetaBgFirstMetaBgFrontRegion]==] [==[Ext4Engine.MetaBgSuperblockFieldsParsed]==] [==[Ext4Engine.BackupSuperblockRescueWhenPrimaryDestroyed]==] [==[Ext4Engine.BackupRescueFallsBackToBackupGdt]==] [==[Ext4Engine.BackupRescueWholeDiskRebase]==] [==[Ext4Engine.BadMagicWithoutBackupsStillRejected]==] [==[Ext4Engine.JournalSalvagedDirReparented]==] [==[Ext4Engine.JournalUnreferencedOrphanFlatWithRealName]==] [==[Ext4Engine.TwoPassJournalClaimInteraction]==] [==[Ext4Engine.CatchBufferBacktrackSalvage]==] [==[Ext4Engine.BacktrackOffZeroFillsTheWindow]==] [==[Ext4Engine.ZeroFillOnlyWhenBacktrackAlsoFails]==] [==[Ext4Engine.NanosecondsPreservedFromExtraField]==] [==[Ext4Engine.InvalidUtf8NameCarriesRawBytes]==] [==[HfsPlus.VolumeHeaderValidation]==] [==[HfsPlus.EndToEndWalk]==] [==[HfsPlus.DecmpfsInlineAndRsrcContentRecovered]==] [==[HfsPlus.DecmpfsDisabledLeavesEmptyShell]==] [==[HfsPlus.MacTimeConversion]==] [==[HfsPlus.BadSignatureRejected]==] [==[HfsPlus.LeafNodeLayoutMatchesOnDiskFormat]==] [==[HfsPlus.DotfileAndPrivateDataFiltered]==] [==[HfsPlus.PrdfRecordFolderized]==] [==[HfsPlus.RecordBit29SelectsResourceFork]==] [==[HfsPlus.LargeBlockSizeAccepted]==] [==[HfsPlus.ExtentsOverflowRecordResolvesFragmentedFork]==] [==[HfsPlus.JournalHarvestEmitsDeletedRecords]==] [==[HfsPlus.JournalDifferentialDifferentFileIdKept]==] [==[HfsPlus.JournalBadHeaderGates]==] [==[HfsPlus.JournalWrapHandled]==] [==[HfsPlus.JournalHarvestDisabledByFlag]==] [==[HfsPlus.JournalSameKeyDifferentFileIdBothKept]==] [==[HfsPlus.JournalAttrDecmpfsFallbackFeedsEmptyForkFile]==] [==[HfsPlus.JournalRequiresVolumeJournaledBit]==] [==[HfsPlus.JournalInfoBlockHasNoMagicField]==] [==[HfsPlus.JournalNodeImageRejectsInconsistentOffsetTable]==] [==[HfsPlus.JournalFallbackSalvageWhenHeaderInvalid]==] [==[HfsPlus.JournalFallbackRequiresNodeShapedStart]==] [==[Btrfs.SuperblockValidation]==] [==[Btrfs.NonPow2NodesizeRejectedRegistered]==] [==[Btrfs.EndToEndWalk]==] [==[Btrfs.TwoSubvolumesRootSeparately]==] [==[Btrfs.SubvolumeNamedFromRootRef]==] [==[Btrfs.FileAttributeNormalBit]==] [==[Btrfs.BadMagicRejected]==] [==[Btrfs.PackedDirItemsInOneItem]==] [==[Btrfs.MirrorSelectionNewestGeneration]==] [==[Btrfs.InternalKeyPtrWalk]==] [==[Btrfs.InlineAndCompressedExtents]==] [==[Btrfs.NonAsciiNamesDecodedNotByteLifted]==] [==[Btrfs.Latin1NameRoundTripsRawBytes]==] [==[Btrfs.XattrDosAttribApplied]==] [==[Btrfs.UserXattrRecordsEmitted]==] [==[Btrfs.CompressionPropertyTagged]==] [==[Btrfs.Raid10GeometryGate]==] [==[Btrfs.Raid1RotatingMirrorReads]==] [==[Btrfs.Raid0StripeMathSplitsAtStripeBoundaries]==] [==[Btrfs.DefaultDirItemNonSubvolumeLocationFallsBack]==] [==[Zfs.EndToEndWalk]==] [==[Zfs.ChildDirZapReadsKernelOffset32]==] [==[Zfs.OriginObjNotTreatedAsChildZap]==] [==[Zfs.LabelPositionsRecoveredFromTail]==] [==[Zfs.NewestTxgAcrossLabelsWins]==] [==[Zfs.ChecksumFailDropsAndRetriesNextDva]==] [==[Zfs.StripedPoolForeignVdevFallbackRead]==] [==[Zfs.MirrorSecondMemberScan]==] [==[Zfs.SaRegistryDrivenLayout2]==] [==[Zfs.SaRegistryAbsentFallsBackToLayout1]==] [==[Zfs.BadMagicRejected]==] [==[Zfs.ZleRoundTrip]==] [==[Zfs.CodecMapping]==] [==[Zfs.CodecRoundTrips]==] [==[Zfs.TruncatedSourcesRejectedNotOverread]==] [==[Zfs.NonAsciiNamesDecodedNotByteLifted]==] [==[RefsDetect.FindsMsbPages]==] [==[RefsDetect.CheckpointVariant]==] [==[RefsDetect.IdentificationFallback]==] [==[RefsDetect.NonRefsRejected]==] [==[Ldm.ParsesDatabaseAndGraph]==] [==[Ldm.ResolvesSimpleVolume]==] [==[Ldm.SimpleVolumeDeviceReads]==] [==[Ldm.StripedVolumeInterleave]==] [==[Ldm.CompositeVblkRecord]==] [==[Ldm.NonLdmDiskRejected]==] [==[Ldm.Raid5LeftSymmetricReads]==] [==[Ldm.Raid5ReconstructsMissingColumn]==] [==[Ldm.Raid5TwoMissingColumnsUnrecoverable]==] [==[Ldm.MirroredReadFailover]==] [==[Ldm.MirroredBothCopiesFail]==] [==[Ldm.SpannedConcatenation]==] [==[Ldm.GptDynamicDiskCanonicalGuidsParse]==] [==[Ldm.GptDynamicDiskFabricatedGuidsRejected]==] [==[Ldm.GptDynamicDiskTableClassifiesLdmEntries]==] [==[Ldm.GptDynamicDiskRealCanonicalGuidsParse]==] [==[Ldm.GptPathFindsMetadata4Kn]==] [==[Ldm.TocblockAtConfigPlus0]==] [==[Ldm.TocblockPrefersPlus2OnConflict]==] [==[Ldm.TocblockTailCopies]==] [==[Ldm.OversizeConfigRejected]==] [==[Ldm.ConfigSizeExactlyOneMebibyteAccepted]==] [==[Ldm.ZeroInterleaveDefaultsTo64K]==] [==[LostDiscovery.FindsScoresAndRejectsNoise]==] [==[LostDiscovery.OverlapClassification]==] [==[LostDiscovery.TwoHealthyDeletedPartitionsBothSurface]==] [==[LostDiscovery.DamagedBpbFallbackMustNotSwallowPeer]==] [==[LostDiscovery.Ext4SuperblockAnchorsDeletedPartition]==] [==[LostDiscovery.Ext4BlockGroupCopyNotACandidateStart]==] [==[LostDiscovery.HfsPlusVolumeHeaderAnchorsDeletedPartition]==] [==[LostDiscovery.BtrfsSuperblockAnchorsDeletedPartition]==] [==[LostDiscovery.ZfsLabelAnchorsDeletedPartition]==] [==[LostDiscovery.RefsBootAnchorsDeletedPartition]==] [==[LostDiscovery.SuperblockNoiseRejected]==] [==[LostDiscovery.LostBitlockerPartitionDiscovered]==] [==[LostDiscovery.LostBitlockerOemNoise]==] [==[LostDiscovery.LostBitlockerInsideExtendedRegionStillFound]==] [==[LostDiscovery.FatBootWithOgGatesBecomesCandidate]==] [==[LostDiscovery.Fat32ExtentUsesDataAreaForContainment]==] [==[LostDiscovery.LostCandidateReportsDataAreaOffset]==] [==[LostDiscovery.FileRichCandidateOutranksCleanBoot]==] [==[LostDiscovery.TableMatchBonus500]==] [==[LostDiscovery.OverLargeDuplicateRescoredToFront]==] [==[LostFatGates.Spc256Rejected]==] [==[LostFatGates.NumFatsZeroRejectedThreeAcceptedByDeepGates]==] [==[LostFatGates.Fat32NameStringTornYieldsNoCandidate]==] [==[LostFatGates.Fat12SmallFatProbe]==] [==[LostFatGates.Fat12ProbeSpfBoundIsExclusiveAt200]==] [==[LostFatGates.Fat32GateStringIsCaseSensitive]==] [==[LostScoring.MatchesLogRows]==] [==[Sha256.NistVectors]==] [==[Sha256.StreamedMatchesOneShot]==] [==[Sha256.Id128Truncation]==] [==[AesXts.Aes128Fips197KnownAnswer]==] [==[AesXts.Aes256Fips197KnownAnswer]==] [==[AesXts.Xts128CrossImplementationVector]==] [==[AesXts.Xts256CrossImplementationVector]==] [==[AesXts.Xts256WholeSectorVector]==] [==[AesXts.WeakKeyHalvesRejected]==] [==[AesXts.RoundTripVariedLengths]==] [==[AesXts.DistinctTweaksGiveDistinctCiphertexts]==] [==[DecryptingDevice.ReadsPlaintextThroughWrapper]==] [==[DecryptingDevice.UnalignedAndCrossSectorReads]==] [==[DecryptingDevice.InnerErrorsPropagateVerbatim]==] [==[DecryptingDevice.BadKeyMaterialCreateFails]==] [==[BitLockerStack.EncryptedVolumeScansOnlyThroughDecryptDevice]==] [==[Dict.ShippedTableMatchesCorpusShape]==] [==[Dict.ShippedTableKeepsCorpusDuplicates]==] [==[Dict.ParseDictionaryGrammar]==] [==[Dict.ParseToleratesLfAndEmptyLines]==] [==[Dict.ParseRoundTripsTheCorpusGrammar]==] [==[Dict.ParseRejectsBadGrammar]==] [==[Dict.TryStopsAtFirstSuccessInShippedOrder]==] [==[Dict.UserCandidatesFollowShippedDictionary]==] [==[Dict.AllFailReportsEveryAttempt]==] [==[Dict.NullAttemptFunctionIsANoOp]==] [==[Dict.AttemptsLoggedThroughSeamWithoutPasswordMaterial]==] [==[ResumeRevalidation.ResumeDropsStaleCandidateAfterRelayout]==] [==[ResumeRevalidation.ResumeKeepsValidCandidate]==] [==[ResumeRevalidation.DiskLayoutRecordRoundTrip]==] [==[ResumeRevalidation.LayoutChangedFlagged]==] [==[ResumeRevalidation.SizeSourceFlagRoundTrip]==] [==[ResumeRevalidation.CandidateGeometryRoundTrip]==] [==[ResumeRevalidation.ArbiterStartOffDiskDropsAndUnknownKeeps]==] [==[Text.AsciiPassesThrough]==] [==[Text.LatinAccentDecodes]==] [==[Text.CjkDecodesToCodePoints]==] [==[Text.AstralPlaneBecomesSurrogatePair]==] [==[Text.OverlongFormsRejectedAsReplacement]==] [==[Text.SurrogateHalvesRejected]==] [==[Text.BeyondUnicodeRejected]==] [==[Text.InvalidLeadBytesResyncOneByte]==] [==[Text.TruncatedTailBecomesReplacement]==] [==[Text.MaximalSubpartReplacement]==] [==[Text.EmbeddedNulDecodes]==] [==[Text.RoundTripThroughNodeUtf8]==] [==[Text.InvalidByteLosslessMapping]==] [==[Text.LosslessRoundTripRecoversRawBytes]==] [==[Text.ZfsDecoderStaysReplacement]==] [==[Text.WideToUtf8EncodesNonAscii]==] [==[Text.WideToUtf8SurrogatePairAndLoneHalf]==] [==[Text.CliFilterUtf8MatchesDecodedNames]==] [==[CarveEngine.GrammarValidationAndSpecificity]==] [==[CarveEngine.CarvesMidSectorStart]==] [==[CarveEngine.SecondPassFullyDeduped]==] [==[CarveEngine.VideoPostProcessorHookInvoked]==] [==[CarveEngine.HookDegradesWhenModuleAbsent]==] [==[CarveEngine.RegistryF8Gate]==] [==[CarveEngine.PstAndRar5Grammar]==] [==[CarveEngine.ExtendedFormatsGrammar]==] [==[CarveEngine.PdfEofLastWinsAndFrameFloors]==] [==[CarveEngine.RawSignLayerHeaderOnlyAndSharedMagic]==] [==[CarveEngine.RawSignTailPairWithinFormat]==] [==[CarveEngine.UnreadableWindowNotCountedAsScanned]==] [==[CarveEngine.ZipCdAnchorCarvesDestroyedHead]==] [==[CarveEngine.ZipIndexlessLfhChainSalvage]==] [==[CarveEngine.ZipSalvageAbsorbsDataSectors]==] [==[CarveEngine.Zip64ExtraFieldContinuesChain]==] [==[CarveEngine.ZipCleanArchiveNotInflatedByZeroSlack]==] [==[CarveEngine.BeTiffAndOrfVariantsCarve]==] [==[CarveEngine.OverlapScorePrefersHigherConfidence]==] [==[CarveEngine.TiffVsRawTiebreak]==] [==[CarveEngine.PstUnknownVersionAcceptedAtFloor]==] [==[CarveEngine.JpegSof0PrecisionGate]==] [==[CarveEngine.SlicedDeviceSecondPassFullyDedupedAndAbsolute]==] [==[CarveEngine.CarvedNodesAreDeleted]==] [==[CarveEngine.RawSignCarvedNodesAreDeleted]==] [==[CarveEngine.DeletedOnlyQueryIncludesCarvedFiles]==] [==[CarveEngine.CarvedNodesCarryScanTimestamp]==] [==[CarveEngine.RawSignCarvedNodesCarryScanTimestamp]==] [==[CarveEngine.ScanTimestampSurvivesSessionRoundTrip]==] [==[CarveEngine.ZipInnerEntryInventoryEmitted]==] [==[CarveEngine.ZipIndexlessEntryInventory]==] [==[CarveEngine.Rar4InnerEntryInventory]==] [==[CarveEngine.ProvenanceChecksumAndAnchorsPersist]==] [==[CarveEngine.TailAnchorCarriedOnTailPairedHits]==] [==[CarveEngine.BitmapSkipsUsedClusters]==] [==[CarveEngine.NoBitmapCarvesEverything]==] [==[CarveEngine.FormatDisabledSkipsSignature]==] [==[CarveEngine.FormatDisabledRawSignsToo]==] [==[CarveEngine.AllFormatsEnabledByDefault]==] [==[CarveEngine.WindowServingEliminatesPerCandidateRereads]==] [==[CarveEngine.WindowServingServesStraddlingReadsCorrectly]==] [==[CarveEngine.Mp4NamedByMetadataTemplate]==] [==[CarveEngine.MovNamedByMetadataTemplate]==] [==[CarveEngine.MovV1MvhdHighWordQuirkReplicated]==] [==[CarveEngine.FallbackNamerDocumented]==] [==[CarveEngine.ZeroSectorSkipStats]==] [==[CarveEngine.WindowBufferReusedByValidatorChecksumAndHash]==] [==[CarveEngine.BoundaryStraddlingCandidateSplitsReadsCorrectly]==] [==[RawSigns.UnitsAreKib]==] [==[RawSigns.CarvesFiveMiBVob]==] [==[RawSigns.MinFloorOneKib]==] [==[RawSigns.TailPairWindowScaled]==] [==[Carve.Mp4RealFtypIsom]==] [==[Carve.MovRealFtypQt]==] [==[Carve.HeicRealFtypHeic]==] [==[Carve.M4aRealFtyp]==] [==[Carve.MovMoovWithoutFtyp]==] [==[Carve.RegistrySelfProbeReachesEveryFormat]==] [==[Carve.S007CommonFormatsCarve]==] [==[Carve.S007ValidatorsRejectNoise]==] [==[Carve.S007TruncatedSalvageSemantics]==] [==[Carve.ZipTruncatedHeadRecovered]==] [==[Carve.ZipLfhSanityGates]==] [==[Carve.MkvDocTypeWebmAccepted]==] [==[Carve.MkvDocTypeExoticRejected]==] [==[Carve.MkvElementBoundsGate]==] [==[Carve.HeifSiblingBrandLabeledHeif]==] [==[Carve.IiuTiffEpValidatedByTiffGrammar]==] [==[Carve.IiuRw2FallbackWhenTiffEpRejects]==] [==[Carve.RafVariantHeaderAccepted]==] [==[Carve.RafRealAsciiVersionAccepted]==] [==[Carve.JpegFourthByteOutsideOgSet]==] [==[Carve.JpegTruncatedTailSalvagedAtSofEstimate]==] [==[Carve.JpegTruncatedTailWithoutSofSalvagesAtCapTenth]==] [==[Carve.JpegCompleteFileStopsAtEoiNotSofEstimate]==] [==[Carve.CabLowercaseMagic]==] [==[Crsf.RoundTrip]==] [==[Crsf.CorruptionSkipsForward]==] [==[Crsf.TreeRoundTrip]==] [==[Crsf.TinyChunkDeflatesWhenSmaller]==] [==[Crsf.VolumeInfoIdentityCardRoundTrip]==] [==[Crsf.TornCheckpointSalvagesPrefix]==] [==[SessionTree.RefreshStatsIsRecursive]==] [==[SessionTree.ReloadedRootStatsAreRecursive]==] [==[SessionTree.StatsDeletedCountRecursive]==] [==[Session.GradeRoleAndProvenanceRoundTrip]==] [==[Session.OldFormatStillLoads]==] [==[Session.ForwardParentDeferredAndCounted]==] [==[Session.BadMarkerFailsTheRecord]==] [==[Session.TruncatedTailFailsAfterFullCount]==] [==[Session.HostileNameAndRoleLengthBombs]==] [==[Session.CorruptRecordEarlyExitsReleaseProps]==] [==[Session.RawNameRoundTrip]==] [==[Session.OldFormatWithoutRawNameStillLoads]==] [==[Session.SaveEmitsProgress]==] [==[Session.SaveCancelsCleanly]==] [==[Session.AutoNamingConvention]==] [==[Session.BtrfsExtrasRoundTrip]==] [==[Session.ZfsSecondaryFlagsRoundTrip]==] [==[Session.EngineExtrasAbsentWhenUnpopulated]==] [==[Orchestrator.DynamicDiskVolumeScan]==] [==[Orchestrator.JunkFilteredFromResults]==] [==[Orchestrator.LogSilentByDefault]==] [==[Orchestrator.LogSinkReceivesScanBrackets]==] [==[Orchestrator.MemoryMonitorEmitsUnderPressure]==] [==[Orchestrator.WeightedProgress]==] [==[Orchestrator.ProbeFamily]==] [==[Orchestrator.SpoofedExfatOemRejectedBeforeEngine]==] [==[Orchestrator.SpoofedFatBootUnreadableRootRejected]==] [==[Orchestrator.RealExfatBootContentVerified]==] [==[Orchestrator.FsFamilyValuesMatchContract]==] [==[Orchestrator.UdfProbeFailsOnMkudffsShapeLikeTheOg]==] [==[Orchestrator.ProbesUdfVolume]==] [==[Orchestrator.UdfProbeRequiresBothDescriptorReads]==] [==[Orchestrator.UdfNotConfusedWithFat]==] [==[Orchestrator.UdfSurfacesAsCarveFamily]==] [==[Orchestrator.CarveSingleNamingFamily]==] [==[Orchestrator.StagedRunAndSessionRoundTrip]==] [==[Orchestrator.PreScanRepairRestoresTornFatAndScans]==] [==[Orchestrator.PreScanRepairDeniedNeverWrites]==] [==[Orchestrator.PreScanRepairWithoutHooksNeverWrites]==] [==[Orchestrator.ReadPipeBuildsCacheOverRetryForValidGeometry]==] [==[Orchestrator.ReadPipeDegradesToBareSliceWhenCacheCreationFails]==] [==[Orchestrator.ReadPipeZeroCapacityDegradesToBareSlice]==] [==[FilterFolder.CorpusListsPinned]==] [==[SafeWriter.OverlapRefusal]==] [==[SafeWriter.CollisionAndSanitize]==] [==[SafeWriter.ResidentPrefixNotOverwritten]==] [==[SafeWriter.InvalidUtf8NameRestoredFromRaw]==] [==[ScanEvents.ErrorRangeGoesToLiveSink]==] [==[ScanEvents.SinkSwitchDeliversAllTypes]==] [==[ScanEvents.SingleBoundaryAtTeardown]==] [==[ScanEvents.ErrorRangeCarriesModuleOperationAttempts]==] [==[ScanEvents.ProbeBlockFailureAttributesToOrchestrator]==] [==[ScanEvents.CarveErrorRangeAttributesToCarveModule]==] [==[ScanConfig.DeepStageDefaultsOnAndWholeDiskOff]==] [==[PauseResume.TokenCheckpointBlocksAndResumes]==] [==[PauseResume.CancelWhilePausedWakesWaiter]==] [==[PauseResume.PausedMsAccounting]==] [==[PauseResume.ScanPauseFreezesAndElapsedExcludesPause]==] [==[PauseResume.SaveOnCancelStampsTruthfulMaskAndResumeSkipsQuick]==] [==[PauseResume.MaskIntegrityViolationRefusesResume]==] [==[PauseResume.ResumedSessionPreservesOriginalConfiguredMask]==] [==[PauseResume.SessionRestoresVolumeInfoAndCandidates]==] [==[PauseResume.SaveAfterResumeReloadsCleanly]==] [==[Checkpoint.StageBoundaryCheckpointResumesAfterCrash]==] [==[Checkpoint.CorruptMagicRefused]==] [==[Checkpoint.CommitHookFiresAfterEveryFinalizeAndAbortsOnError]==] [==[ExtentBasis.TreeRebaseShiftsOnceAndIsIdempotent]==] [==[ExtentBasis.SliceScanSavesAbsoluteExtents]==] [==[ExtentBasis.LegacySliceRelativeSessionNormalizedOnLoad]==] [==[Telemetry.UniformProgressAcrossStagesMonotonic]==] [==[Telemetry.FatWalkEmitsIncrementalTicks]==] [==[Telemetry.PerStageShapeAndCompletion]==] [==[Telemetry.LiveErrorMapVisibleMidScan]==] [==[Telemetry.QueryDuringScanIsThreadSafe]==] [==[WriterDirs.StructureReconstructionWithEmptyLeaves]==] [==[WriterDirs.DeepDestChainCreatedAndIdempotentRerun]==] [==[WriterDirs.IntraQueueDuplicatesRenameUnderSkip]==] [==[WriterDirs.PerItemErrorCollectedQueueContinues]==] [==[WriterDirs.MetadataOnlyItemSkipsNotZeroFills]==] [==[WriterDirs.ProgressExcludesErrorItems]==] [==[WriterDirs.CollisionNameUnderscoreFormat]==] [==[WriterDirs.CollisionExhaustionFailsTheItem]==] [==[WriterDirs.MkdirSemanticsMatchShCreateDirectoryEx]==] [==[Reghive.ParsesValidHiveAndRejectsGarbage]==] [==[Reghive.RejectsCorruptedHeaderChecksum]==] [==[Reghive.WalksControlSetKeyFamilyCaseInsensitive]==] [==[Reghive.EnumeratesSubkeyNames]==] [==[Reghive.ReadsBinaryValueDataPointedAndInline]==] [==[Reghive.UnicodeKeyNamesWalkAsAscii]==] [==[Reghive.TruncatedAndMutatedHivesParseOrRefuseWithoutCrashing]==] [==[FveAutoUnlock.HarvestsPerGuidDataBlobs]==] [==[FveAutoUnlock.HarvestCapsBlobsAt256Bytes]==] [==[FveAutoUnlock.HarvestCapsRecordsAt100AndSkipsTinyBlobs]==] [==[FveAutoUnlock.SelectionPrefersMatchingVolumeGuid]==] [==[Pwddict.ParsesOgeDictFormatByteFaithfully]==] [==[Pwddict.ParseAcceptsLfAndOptionalComma]==] [==[Pwddict.ParseEnforcesCapsAndRefusesOversize]==] [==[Pwddict.SeedDictionaryIsSane]==] [==[Pwddict.TryPasswordsStopsAtFirstSuccess]==] [==[Pwddict.TryPasswordsDedupsPreservingOrder]==] [==[Pwddict.TryPasswordsExhaustsToLockedAndGuardsArgs]==] [==[Pwddict.AttemptLogCarriesProgressAndLengthNeverSecrets]==] [==[Pwddict.ExhaustLogsWarnOnce]==] [==[Preview.ExtentAccurateReadsWithSparseTailAndEof]==] [==[Preview.BoundedReadCannotBeTricked]==] [==[Preview.ReadFailuresZeroFilledAndReported]==] [==[Preview.CompressedExtentDecodes]==] [==[Preview.ConcurrentIndependentReaders]==] [==[Preview.MalformedCapsExtentKeepsAlignment]==] [==[PreviewMeta.JpegDimensionsAndExif]==] [==[PreviewMeta.PngGifBmpDimensions]==] [==[PreviewMeta.WavDurationAndMp4Brand]==] [==[PreviewMeta.PdfTextAndPlainAndUnsupported]==] [==[PreviewMeta.TruncatedMagicsAreSafeAndHonest]==] [==[PreviewMeta.WavFmtChunkOutsideHeadCache]==] [==[PreviewMeta.DocxRawDeflateInflatesAndPlainZipStaysZip]==] [==[PreviewMeta.DocxCapsMaxReadHonored]==] [==[PreviewMeta.DocxTailBeyondBudgetDegradesToZip]==] [==[PreviewMeta.CompressedExtentReportsTruncatedUnderBudget]==] [==[PreviewMeta.CompressedExtentWholeBlockReadDisclosed]==] [==[PreviewMeta.MetadataTruncatedFlagTruthfulAllPaths]==] [==[PreviewMeta.RandomSmallInputsAreSafe]==] [==[Snapshot.CaptureFidelityAndDfsParity]==] [==[Snapshot.PartialFlagAndEmptyTree]==] [==[Snapshot.ConcurrentEmitCaptureWalk]==] [==[Snapshot.RecoveryFromSnapshotIsByteIdentical]==] [==[Query.CategoryMap]==] [==[Query.EmailCategoryExists]==] [==[Query.BookmarksCategory]==] [==[Query.CameraRawClassified]==] [==[Query.TableCountMatchesCorpusUniques]==] [==[Query.CrossListedFirstWriterWins]==] [==[Query.FilterSortPageCorrectness]==] [==[Query.PathPrefixFilterMatchesAndPrunes]==] [==[Query.RandomizedAgainstReference]==] [==[Query.TypeSortKeyOrdersByCategoryThenExtension]==] [==[Query.ModifiedUnitIsFiletimeEverywhere]==] [==[Query.DateBucketsTranslateToRanges]==] [==[Query.SizeBucketsTranslateToRanges]==] [==[Query.BucketFiltersThroughRun]==] [==[Error.ErrTextCoversEveryCode]==] [==[QueryPerf.MillionNodesFilterCountUnder200ms]==] [==[DiskMap.RangeFreeEdgeMatrix]==] [==[DiskMap.FreeMapRecordRoundTripAndRejection]==] [==[DiskMap.FreeOverlayHonorsZoomWindow]==] [==[DiskMap.DownsamplePreservesRawTruth]==] [==[DiskMap.FreeNeverMarksFoundContentCells]==] [==[DiskMap.OverlayBudgetGrowsCellSize]==] [==[Apply.PlansEntryIntoFirstFreeSlot]==] [==[Apply.ExecuteWritesEntryVerifiesAndUndoes]==] [==[Apply.RefusedWithoutConsent]==] [==[Apply.StaleTableRefusedBeforeConsent]==] [==[Apply.QuiesceFailureIsACleanRefusal]==] [==[Apply.WriteFailureRollsBackEverything]==] [==[Apply.ContainmentRefusals]==] [==[Apply.SignatureRestorePlannedWhenAbsent]==] [==[Apply.ChsClampsBeyondCylinder1023]==] [==[Apply.FamilyTypeMap]==] [==[LostApply.WritesEntryAndRefreshesModernPath]==] [==[LostApply.LegacyPathMatchesCorpusSequence]==] [==[LostApply.GptRebuildRegeneratesSignature]==] [==[LostApply.ConsentBeforeLockDenialHoldsNothing]==] [==[LostApply.UndoApplyRestoresAndRefreshes]==] [==[LostApply.RefreshFailureIsReportedNotSwallowed]==] [==[LostApply.RollsBackWhenVerifyFails]==] [==[LostApply.PlanIsReadOnlyAndRefusalsAreResults]==] [==[RawFixer.CleanExfatNeverTouched]==] [==[RawFixer.ExfatChecksumRepair]==] [==[RawFixer.ExfatDestroyedPrimaryRestoredFromBackup]==] [==[RawFixer.NtfsBackupRestore]==] [==[RawFixer.NtfsCleanRefused]==] [==[RawFixer.NtfsNoBackupRefused]==] [==[RawFixer.Fat32BackupRestore]==] [==[RawFixer.ExfatGeometryRederivation]==] [==[RawFixer.ExfatAmbiguousGeometryRefused]==] [==[RawFixer.UnknownFamilyRefused]==] [==[RawFixer.ExecuteRefusesEmptyPlan]==] [==[RawFixer.FailedWriteRollsBackEverything]==] [==[RawFixer.UndoLogRoundTrip]==] [==[RawFixer.MftRecord0RestoredFromMirror]==] [==[RawFixer.MftCleanNeverTouched]==] [==[RawFixer.MftBothInvalidNoWrite]==] [==[RawFixer.MftRestoreMakesVolumeOpenable]==] [==[RawFixer.FatTornMirrorCopyRestoredSectorGranular]==] [==[RawFixer.FatTornPrimaryRestoredFromMirror]==] [==[RawFixer.FatMirrorAmbiguityNeverWritten]==] [==[RawFixer.FatCleanMirrorNoAction]==] [==[RawFixer.FixByScanSectorRebuildsBootFromOnlyCandidate]==] [==[RawFixer.FixByScanSectorAmbiguousCandidatesRefused]==] [==[RawFixer.FixByScanSectorIdenticalCandidatesNotAmbiguous]==] [==[RawFixer.RebasedPartitionPlanWritesAtDiskBasisNeverLba0]==] [==[RawFixer.VolumeLockAcquiredBeforeFirstWrite]==] [==[RawFixer.VolumeLockFailureNeverWrites]==] [==[RawFixer.UnlockHappensOnRollbackPath]==] [==[RawFixer.NtfsPrimaryDamaged4Kn]==] [==[RawFixer.ExfatViaBackup4Kn]==] [==[RawFixer.AnalyzeAllBranches4KnAsan]==] [==[RawFixer.ExfatRestorePlanIsSectorSized4Kn]==] [==[RawFixer.ExfatBackupReadFull4Kn]==] [==[Consistency.ExtensionIsLastDotUppercased]==] [==[Consistency.CheckableVocabularyIsSignatureFamiliesPlusSpecials]==] [==[Consistency.ClassifierIsSubstringPlusVideoSpecials]==] [==[Lost.ContentInconsistentGhostRejected]==] [==[Lost.ContentConsistentGhostAccepted]==] [==[Lost.RatioBoundaryThirtyPercent]==] [==[Lost.MajorityGateRequiresHalfFilterable]==] [==[Lost.DirectoriesAndExtentlessFilesMirrorOgCounters]==] [==[Lost.DataStartOutOfBoundsInvalidatesTheCandidate]==] [==[Lost.UnreadableDataStartStaysUnmatched]==] [==[Lost.EmptyCandidateListNeverRejects]==] [==[Lost.WholeDiskScanDropsContentInconsistentExfatGhost]==] [==[Lost.WholeDiskScanKeepsContentConsistentExfatGhost]==] [==[TreeDetach.RemovesNodesRootsAndIndexes]==] [==[TreeDetach.HardLinkPropsSurviveWithTheirRefs]==] [==[Refs.BootValidation]==] [==[Refs.FullScanHierarchy]==] [==[Refs.FallbackRootSkipsOgSystemFamilies]==] [==[Refs.DeletedGradingAndId2]==] [==[Refs.OrchestratorProbeAndScan]==] [==[Refs.NextChainFindsNewerTable]==] [==[Refs.CheckpointLayoutMinor14]==] [==[Refs.Pre34CheckpointRejected]==] [==[Refs.CheckpointBodyMinorGatesLayoutNotVbr]==] [==[Refs.VbrMinor14BodyMinorOldListStillOldLayout]==] [==[Refs.ResidentInlineFileRecovered]==] [==[Refs.ResidentDollarIMetadataRead]==] [==[Refs.MultipleDataAttributesMerged]==] [==[Refs.PropertyFlagAndUnknownRowsDoNotBreakDataWalk]==] [==[Refs.PagedRunTableFollowed]==] [==[Refs.Gen7OldGrammarDatarunAttribute]==] [==[Refs.RunlistGranularityMismatchDiscarded]==] [==[Refs.StaleMsbPageDeletedFileRecovered]==] [==[Refs.StaleSweepSkipsLivePages]==] [==[Refs.HardLinkRowsCollapseToOneResult]==] [==[Refs.RecycleRoutedFlaggedAndDollarIRemap]==] [==[Refs.ProgressMonotonicIntermediateTicks]==] [==[Refs.ProgressCancellationMidWalk]==] [==[Refs.Bps256Accepted]==] [==[Refs.Bps1024Accepted]==] [==[Refs.Cluster16KRejected]==] [==[Refs.Cluster64KAccepted]==] [==[Refs.OpenReadIsFourKiB]==] [==[Refs.ContainerRelativeRunTranslatedPerOgFormula]==] [==[Refs.PhysicalRunFlag20Untranslated]==] [==[Refs.ContainerBlockBelowSpanStaysPhysical]==] [==[Refs.SingleContainerExtentsUnchanged]==])
