// Needs to be included when not in .ino - for uint8_t
#include <stdint.h>
#include "time.h"

/*
 * Nanopb protobuf
 */
#include "pb.h"
#include "pb_encode.h"
#include "pb_common.h"

/*
 * Compiled OpenTelemetry Proto headers
 */
#include "metrics.pb.h"
#include "common.pb.h"
#include "resource.pb.h"

// Protobuf setup
#define MAX_PROTOBUF_BYTES 4096

// Attributes struct
typedef struct anode *Attrptr;
typedef struct anode
{
    char *key;
    char *value;
} Attrnode;

// Datapoints
typedef struct dnode *Dpointsptr;
typedef struct dnode
{
    uint64_t time;
    int type;
    union
    {
        int64_t as_int;
        double as_double;
    } value;
    Attrptr attributes;
} Dpointnode;

// Metrics
typedef struct mnode *Metricsptr;
typedef struct mnode
{
    char *name;
    char *description;
    char *unit;
    Dpointsptr datapoints;
} Metricnode;

// Resource + Scope
typedef struct rnode *Rmetricsptr;
typedef struct rnode
{
    Attrptr attributes;
    Metricsptr metrics;
} Rmetricsnode;

// To enumerate the metric value types
enum
{
    AS_INT,
    AS_DOUBLE
};

uint64_t getEpochNano(void);

// Protobuf encoding of a string
bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void *const *arg);

// Protobuf encoding of Key-Value pairs (Attributes in OpenTelemetry)
bool KeyValue_encode_attributes(pb_ostream_t *ostream, const pb_field_iter_t *field, void *const *arg);

// Protobuf encoding of a Sum datapoint
bool Sum_encode_data_points(pb_ostream_t *ostream, const pb_field_iter_t *field, void *const *arg);

// Protobuf encoding of a Metric definition
bool ScopeMetrics_encode_metric(pb_ostream_t *ostream, const pb_field_iter_t *field, void *const *arg);

// Protobuf encoding of a scope (passthrough - nothing much done here)
bool ResourceMetrics_encode_scope_metrics(pb_ostream_t *ostream, const pb_field_iter_t *field, void *const *arg);

// Protobuf encoding of entire payload
bool MetricsData_encode_resource_metrics(pb_ostream_t *ostream, const pb_field_iter_t *field, void *const *arg);

size_t buildProtobuf(Rmetricsptr args, uint8_t *pbufPayload, size_t pbufMaxSize);

Rmetricsptr createDataPtr(void);

void releaseDataPtr(Rmetricsptr myDataPtr);