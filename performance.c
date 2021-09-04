#include <stdio.h>
#include <stdint.h>
#include <dds/dds.h>
#include <perf.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

int publisher(const char *topic, int n_samples, int sample_size)
{
    dds_entity_t participant;
    dds_entity_t topic_desc;
    dds_entity_t writer;
    dds_return_t res;
    uint32_t status = 0;

    participant = dds_create_participant(DDS_DOMAIN_DEFAULT, NULL, NULL);
    if (participant < 0) {
        return -1;
    }

    topic_desc = dds_create_topic(participant, &performance_performance_profile_desc, topic, NULL, NULL);
    if (topic_desc < 0) {
        return -1;
    }

    writer = dds_create_writer(participant, topic_desc, NULL, NULL);
    if (writer < 0) {
        return -1;
    }

    res = dds_set_status_mask(writer, DDS_PUBLICATION_MATCHED_STATUS);
    if (res != DDS_RETCODE_OK) {
        return -1;
    }

    while (!(status & DDS_PUBLICATION_MATCHED_STATUS)) {
        res = dds_get_status_changes(writer, &status);
        if (res != DDS_RETCODE_OK) {
            return -1;
        }

        usleep(1000);
    }

    performance_performance_profile prof;
    int i = 0;

    prof.n_samples = n_samples;
    prof.sample_size = sample_size;
    prof.buf._buffer = dds_alloc(sample_size);
    prof.buf._length = sample_size;

    for (i = 0; i < n_samples; i ++) {
        struct timespec t;

        clock_gettime(CLOCK_MONOTONIC, &t);
        prof.t.sec = t.tv_sec;
        prof.t.nsec = t.tv_nsec;

        dds_write(writer, &prof);
        //printf("writing samples: %d size: %d sec: %f nsec : %f\n",
        //        prof.n_samples, prof.sample_size, prof.t.sec, prof.t.nsec);
        usleep(1);
    }

    dds_free(prof.buf._buffer);
    return 0;
}


#define PUBLISHER_TOPIC "/publisher"

static struct sample_data {
    int sample_size_byts;
    int samples;
} samples[] = {
    {64, 1000},
    {128, 1000},
    {256, 1000},
    {384, 1000},
    {512, 1000},
    {768, 1000},
    {1024, 1000}
};

int main(int argc, char **argv)
{
    int i;

    for (i = 0; i < sizeof(samples) / sizeof(samples[0]); i ++) {
        publisher(PUBLISHER_TOPIC, samples[i].samples, samples[i].sample_size_byts);
    }
}

