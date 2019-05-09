
Command line interface for the Cassandra cluster:

```
cqlsh
```

(Without sourcing the DAQling build environment!)

Created keyspace with:

```
CREATE KEYSPACE daq WITH replication = {'class': 'SimpleStrategy', 'replication_factor': '1'}  AND durable_writes = true;
```

Dump table content:

```
SELECT * FROM daq.payload;
```

Drop table content:

```
DROP TABLE daq.payload;
```