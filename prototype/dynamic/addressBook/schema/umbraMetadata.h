
struct umbraMetadata {
    float metaDataVersion;
    struct timespec *created;
    struct timespec *updated;
    struct timespec *deleted;
    int deleteflag;
};
