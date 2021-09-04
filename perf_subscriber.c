#include <stdio.h>
#include <stdint.h>
#include <dds/dds.h>
#include <perf.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

static dds_entity_t waitset;
static dds_entity_t reader;

struct perf_sample_data {
    double delta_sec;
    double delta_nsec;
};

static struct perf_sample_data perf_data[1000];

static void timespec_diff(
            double a_sec, double a_nsec,
            double b_sec, double b_nsec,
            double *r_sec, double *r_nsec)
{
    *r_sec  = a_sec  - b_sec;
    *r_nsec = a_nsec - b_nsec;

    if (r_nsec < 0) {
        --r_sec;
        r_nsec += 1000000000L;
    }
}

static void data_available(dds_entity_t rd, void *arg)
{
    int status;
    int sample_count;
    static void *samples[1];
    static dds_sample_info_t info[1];
    int i;
    static int count;

    sample_count = dds_take(rd, samples, info, 1, 1);
    if (sample_count < 0) {
        printf("sample_count %d %s\n", sample_count, dds_strretcode(-sample_count));
        return;
    }

    for (i = 0; !dds_triggered(waitset) && (i < sample_count); i ++) {
        if (info[i].valid_data) {
            struct timespec rx;
            performance_performance_profile *prof = samples[i];
            int ret;

            clock_gettime(CLOCK_MONOTONIC, &rx);
            timespec_diff(rx.tv_sec,
                          rx.tv_nsec,
                          prof->t.sec,
                          prof->t.nsec,
                          &perf_data[count].delta_sec,
                          &perf_data[count].delta_nsec);

            //printf("samples: %d sample_count %d, sec: %f nsec: %f\n",
            //            prof->n_samples, prof->sample_size, prof->t.sec, prof->t.nsec);
            //printf("sample delta_sec: %f delta_nsec: %f\n",
            //            perf_data[count].delta_sec, perf_data[count].delta_nsec);
            count ++;
            if (count == prof->n_samples) {
                double sum = 0;
                double avg = 0;
                int j;

                for (j = 0; j < count; j ++) {
                    sum += (perf_data[j].delta_sec * 1000000.0) +
                           (perf_data[j].delta_nsec / 1000.0);
                    //printf("avg: %f\n", perf_data[j].delta_nsec);
                }

                avg = sum / count;
                printf("samples [%d] [%d] \t avg: %f!\n", prof->n_samples, prof->sample_size, avg);
                count = 0;
            }
        }
    }
}

#define PUBLISHER_TOPIC "/publisher"

int subscriber(const char *topic)
{
    dds_entity_t participant;
    dds_entity_t topic_desc;
    performance_performance_profile prof;
    dds_qos_t *qos;
    dds_listener_t *listener;
    dds_return_t res;

    prof.buf._length = 4096;
    prof.buf._buffer = dds_alloc(4096);
    if (!prof.buf._buffer) {
        return -1;
    }

    participant = dds_create_participant(DDS_DOMAIN_DEFAULT, NULL, NULL);
    if (participant < 0) {
        return -1;
    }

    topic_desc = dds_create_topic(participant, &performance_performance_profile_desc, PUBLISHER_TOPIC, NULL, NULL);
    if (topic_desc < 0) {
        return -1;
    }

    listener = dds_create_listener(NULL);
    dds_lset_data_available(listener, data_available);

    qos = dds_create_qos();
    dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE, DDS_SECS(10));
    reader = dds_create_reader(participant, topic_desc, qos, listener);
    if (reader < 0) {
        return -1;
    }

    waitset = dds_create_waitset(participant);

    res = dds_waitset_attach(waitset, waitset, waitset);
    if (res < 0) {
        return -1;
    }

    while (!dds_triggered(waitset)) {
        dds_attach_t wsresults[1];
        res = dds_waitset_wait(waitset, wsresults, 1, DDS_INFINITY);
        if (res < 0) {
            return -1;
        }
    }
}

int main(int argc, char **argv)
{
    subscriber(PUBLISHER_TOPIC);
}
