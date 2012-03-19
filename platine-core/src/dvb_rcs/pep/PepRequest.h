/**
 * @file PepRequest.h
 * @brief Allocation or release request from a PEP component
 * @author Didier Barvaux / Viveris Technologies
 */

#ifndef PEP_REQUEST_H
#define PEP_REQUEST_H


/**
 * @brief The different type of requests the PEP may send
 */
typedef enum
{
	PEP_REQUEST_RELEASE = 0,     /**< a request for resources release */
	PEP_REQUEST_ALLOCATION = 1,  /**< a request for resources allocation */
	PEP_REQUEST_UNKNOWN = 2,     /**< for error handling */
} pep_request_type_t;



/**
 * @class PepRequest
 * @brief Allocation or release request from a PEP component
 */
class PepRequest
{

 private:

	/** The type of PEP request */
	pep_request_type_t type;
	/** The ST the PEP request is for */
	unsigned int st_id;
	/** The CRA of the PEP request */
	unsigned int cra;
	/** The RBDC of the PEP request */
	unsigned int rbdc;
	/** The RBDCmax of the PEP request */
	unsigned int rbdc_max;


 public:

	PepRequest(pep_request_type_t type, unsigned int st_id,
	           unsigned int cra, unsigned int rbdc, unsigned int rbdc_max);

	~PepRequest();

	pep_request_type_t getType();
	unsigned int getStId();
	unsigned int getCra();
	unsigned int getRbdc();
	unsigned int getRbdcMax();
};

#endif

