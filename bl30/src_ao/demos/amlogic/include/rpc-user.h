#ifndef __RPC_USER_H__
#define __RPC_USER_H__

/*Define Message Type Here*/
/*******************************************************************************
 * Message composition
 ******************************************************************************/

/* ...Message composition with module(6bits), function(10bits) */
#define __MBX_COMPOSE_MSG(mod, func)	(((mod) << 10) | ((func) & 0x3FF))

/* ...accessors */
#define MBX_MSG_MOD(msgcode)	(((msgcode) & 0x3F) >> 10)
#define MBX_MSG_FUNC(msgcode)	((msgcode) & (0x3FF))

/*******************************************************************************
 * Define moudle type here, 6bits valid
 ******************************************************************************/
#define MBX_SYSTEM		0x0

/*******************************************************************************
 * Define function here, 10bits valid
 ******************************************************************************/
	 /*SYSTEM*/
#define CMD_UNDEFINE		0x0
#define CMD_RPCUINTREE_TEST	0x6
#define CMD_RPCUINTTEE_TEST	0x7

/*******************************************************************************
 * Mssage Comopsition
 ******************************************************************************/
#define MBX_CMD_RPCUINTREE_TEST	__MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_RPCUINTREE_TEST)
#define MBX_CMD_RPCUINTTEE_TEST	__MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_RPCUINTTEE_TEST)

#endif
