#!/usr/bin/env python3

import cx_Oracle
import json
import logging

logger = None
db_pool = None


def init(config,log):
    global db_pool
    global logger
    logger=log
    db_pool=cx_Oracle.SessionPool(config["ora_acc"],config["ora_pass"],config["ora_name"], 2, 4, 1, threaded=True)

def initDB():
    conn = db_pool.acquire()
    cursor = conn.cursor()
#    cursor.execute("DROP TABLE faser_ora_dev.RunInfoTable")
    cursor.execute(
    """CREATE TABLE faser_ora_dev.RunInfoTable
  ( 
    "RUNNUMBER" NUMBER NOT NULL PRIMARY KEY , 
    "TYPE" VARCHAR2(200 BYTE) NOT NULL , 
    "VERSION" VARCHAR2(200 BYTE) NOT NULL , 
    "CONFIGNAME" VARCHAR2(200 BYTE) NOT NULL , 
    "START_TIME" TIMESTAMP NOT NULL , 
    "STOP_TIME" TIMESTAMP NOT NULL , 
    "CONFIGURATION" CLOB CHECK (CONFIGURATION is json), 
    "RUNINFO" CLOB CHECK (RUNINFO is json)
  )
    """)

def updateDBConfig():
    conn = db_pool.acquire()
    cursor = conn.cursor()
    cursor.execute(
    """ALTER TABLE faser_ora_dev.RunInfoTable ADD
  ( "USER" VARCHAR2(100 BYTE) DEFAULT 'N/A' NOT NULL ,
    "HOST" VARCHAR2(100 BYTE) DEFAULT 'N/A' NOT NULL ,
    "COMMENT" VARCHAR2(500 BYTE) DEFAULT 'N/A' NOT NULL ,
    "DETECTORS" CLOB DEFAULT '[]' CHECK (DETECTORS is json)
  )
    """)

def insertNewRun(data,first=False):
    conn = db_pool.acquire()
    cursor = conn.cursor()
    if type(data["configuration"])!=str:
        data["configuration"]=json.dumps(data["configuration"])
    if type(data["detectors"])!=str:
        data["detectors"]=json.dumps(data["detectors"])
    if first:
        insertQuery = """insert into RunInfoTable (RUNNUMBER, TYPE, VERSION, CONFIGNAME, START_TIME, STOP_TIME, CONFIGURATION) 
                         values (1, :type, :version, :configName, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, :configuration) 
                      """
    else:
        insertQuery = """insert into RunInfoTable (RUNNUMBER, TYPE, VERSION, CONFIGNAME, START_TIME, STOP_TIME, "USER", HOST, "COMMENT", DETECTORS, CONFIGURATION) 
                         select RUNNUMBER+1, :type, :version, :configName, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, :username, :host, :startcomment, :detectors, :configuration from RunInfoTable
                         where rownum=1 order by RUNNUMBER DESC"""
    try:
        cursor.setinputsizes(configuration=cx_Oracle.CLOB) # need to set to cx_Oracle.CLOB!
        cursor.execute(insertQuery,data)
        cursor.execute("select RUNNUMBER from RunInfoTable where rownum=1 order by RUNNUMBER DESC")
        conn.commit()
        res=cursor.fetchone()
        if not res:
            logger.error("Did not get runnumber")
            return 0
        logger.info("Assigned run number: "+str(res[0]))
        return res[0]

    except cx_Oracle.Error as e:
        logger.error("Failed to insert new run: "+str(e))
        return 0

def addRunInfo(runno,runinfo):
    conn = db_pool.acquire()
    cursor = conn.cursor()

    if type(runinfo)!=str: runinfo=json.dumps(runinfo)
    
    timeQuery = """select START_TIME, STOP_TIME from RunInfoTable where RUNNUMBER=:runno"""
    cursor.execute(timeQuery,runno=runno)
    start,stop=cursor.fetchone()
    if start==stop:
        updateQuery = """update RunInfoTable set STOP_TIME=CURRENT_TIMESTAMP, RUNINFO=:runinfo where RUNNUMBER=:runno"""
    else:
        updateQuery = """update RunInfoTable set RUNINFO=:runinfo where RUNNUMBER=:runno"""
    try:
        cursor.setinputsizes(runinfo=cx_Oracle.CLOB) # need to set to cx_Oracle.CLOB!
        cursor.execute(updateQuery,runno=runno,runinfo=runinfo)
        conn.commit()
    except cx_Oracle.Error as e:
        logger.error(f"Failed to update RunInfo for run {runno}: "+str(e))
        return False
    return True

def getRunInfo(runno):
    conn = db_pool.acquire()
    cursor = conn.cursor()
    infoQuery = """select json_object (
                   'runnumber'  value RUNNUMBER,
                   'type'       value TYPE,
                   'version'    value VERSION,
                   'configName' value CONFIGNAME,
                   'starttime'  value START_TIME,
                   'stoptime'   value STOP_TIME,
                   'username'   value "USER",
                   'host'       value HOST,
                   'startcomment' value "COMMENT"
                   ), configuration,RunInfo,detectors  from RunInfoTable where RUNNUMBER=:runno"""
    try:
        cursor.execute(infoQuery,runno=runno)
        result=cursor.fetchone()
        if not result:
            logger.error("Did not get run information")
            return None
        res,configuration,runinfo,detectors=result #annoyingly json_object doesn't support returning CLOBs
        try:
            data=json.loads(res)
            if runinfo:
                data["runinfo"]=json.loads(str(runinfo))
            else:
                data["runinfo"]=None
            data["configuration"]=json.loads(str(configuration))
            data["detectors"]=json.loads(str(detectors))
        except json.decoder.JSONDecodeError:
            logger.error("Failed to decode json content")
            return None
        return data
    except cx_Oracle.Error as e:
        logger.error("Failed to find run: "+str(e))
        return None


def getRunList(query):
    conn = db_pool.acquire()
    cursor = conn.cursor()

    #TODO: add query requirements

    listQuery = """select json_object (
                   'runnumber'  value RUNNUMBER,
                   'type'       value TYPE,
                   'version'    value VERSION,
                   'configName' value CONFIGNAME,
                   'starttime'  value START_TIME,
                   'stoptime'   value STOP_TIME,
                   'username'       value "USER",
                   'host'       value HOST,
                   'startcomment'    value "COMMENT"
                   ), detectors from RunInfoTable order by runnumber desc"""
    try:
        cursor.execute(listQuery)
        runs=[]
        for row in cursor:
            data=json.loads(row[0])
            data["detectors"]=json.loads(str(row[1]))
            runs.append(data)
        return runs

    except cx_Oracle.Error as e:
        logger.error("Failed to get run list: "+str(e))
        return None
