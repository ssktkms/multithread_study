
{
    time_t now;
    time(&now);
    const struct tm *nowtm = localtime(&now);
    printf("%02d:%02d:%02d\n", nowtm->tm_hour, nowtm->tm_min, nowtm->tm_sec);
}
{
    time_t now;
    struct tm nowtm;
    time(&now);
    localtime_r(&now, &nowtm);
    printf("%02d:%02d:%02d\n", nowtm.tm_hour, nowtm.tm_min, nowtm.tm_sec);
}
