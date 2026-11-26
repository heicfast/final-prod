# The e2e validation PR

This PR exists to drive `e2e-smoke.yml` end to end on a real pull request
(the lane's PR trigger): build the checked-out engine kit, run the 4-row
smoke matrix (CLI path) on real VHDX disks, produce the results artifact.
It carries no product code change by design - it validates the lane itself.
