/*******************************************************************
 *
 *                      Slave SH-2
 *
 * Runs a routine on the slave SH-2 with SBL's SPR_RunSlaveSH.
 *
*******************************************************************/
#ifndef SLAVE_H
#define SLAVE_H

/**
 * @brief Starts a routine on the slave SH-2.
 *
 * @param routine Routine to run.
 * @param parm Passed to the routine.
 */
void slaveRun(void (*routine)(void *), void *parm);

/**
 * @brief Waits for the slave's routine to return.
 */
void slaveWait(void);

#endif
