/*
 * Copyright 2014 Sony Corporation
 */

/**
 * @brief IDD MiscTA inteface
 *
 * @author Leif Mårtensson (leif.martensson@sonymobile.com)
 */

#ifndef IDDD_MISCTA_H_INCLUSION_GUARD
#define IDDD_MISCTA_H_INCLUSION_GUARD

/**
 * Set the miscta flag TA_IDD_REPORT_DATA to 1. This flag was introduced in Kitakami.
 * That indicates that new plugin data has been created.
 * Used by EMMA to decide if the IDD partition shall be extracted and sent.
 */
void miscta_set_report_data();

#endif
