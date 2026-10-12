# `git archive` replaces the placeholder below with the commit hash
# (export-subst in .gitattributes), so that a source tree without .git, such
# as a release tarball or a GitHub source download, still knows its revision.
# In a checkout the placeholder stays as it is and CMakeLists.txt asks git
# instead. See "Package identity" in CMakeLists.txt.
set(ALEPH_ARCHIVED_SOURCE_REVISION "$Format:%H$")
