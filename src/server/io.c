/*
 * =====================================================================================
 *
 *       Filename:  unmap.c
 *
 *    Description:  map an io_forward operation into an 
 *    		    real operation 
 *
 *        Version:  1.0
 *        Created:  12/20/2011 04:53:41 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  drealdal (zhumeiqi), meiqizhu@gmail.com
 *        Company:  Tsinghua University, HPC
 *
 * =====================================================================================
 */
#include <netcdf.h>

#include "io.h"
#include "id.h"
#include "msg.h"
#include "buffer.h"
#include "debug.h"
#include "mpi.h"

extern int client_num;

int iofw_io_client_done(int *client_done, int *server_done)
{
    
    (*client_done)++;

    if((*client_done) == client_nume)
    {
	*server_done = 1;
    }

    debug(DEBUG_IO, "client_done = %d; client_num = %d", 
	    *client_done, client_num);
    
    return IOFW_IO_ERROR_NONE;	
}

int iofw_io_nc_create(int client_id)
{
    int ret, cmode;
    char *path = NULL;
    int ncid, client_ncid;
    iofw_id_nc_t *nc;
    int return_code;
    
    iofw_msg_unpack_nc_create(&path,&cmode, &client_ncid);
    
    if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_nc(client_ncid, &nc))
    {
	ret = nc_create(path,cmode,&ncid);	
	if(ret != NC_NOERR)
	{
	    error("Error happened when open %s error(%s)", 
		    path, nc_strerror(ret));
	    return_code = IOFW_IO_ERROR_NC;

	    goto RETURN;
	}

	iofw_id_map_nc(client_ncid, ncid);
    }

    return_code = IOFW_IO_ERROR_NONE;

RETURN:
    if(NULL != path)
    {
	free(path);
	path = NULL;
    }
    return return_code;
}

int iofw_io_nc_def_dim(int client_id)
{
    int ret = 0;
    int dimid;
    int client_ncid, client_dimid;
    iofw_id_nc_t *nc;
    iofw_id_dim_t *dim;
    size_t len;
    char *name = NULL;
    
    int return_code;

    iofw_msg_unpack_nc_def_dim(&client_ncid, &name, &len, &client_dimid);

    if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_nc(client_ncid, &nc))
    {
	return_code = IOFW_IO_ERROR_INVALID_NC;
	goto RETURN;
    }
    if(IOFW_ID_ERROR_GET_NULL == 
	    iofw_id_get_dim(client_ncid, client_dimid, &dim))
    {
	ret = nc_def_dim(nc->nc_id,name,len,&dimid);
	if( ret != NC_NOERR )
	{
	    error("def dim(%s) error(%s)",name,nc_strerror(ret));
	    return_code = IOFW_IO_ERROR_NC;
	    goto RETURN;
	}
	iofw_id_map_dim(
		client_ncid, client_dimid, nc->nc_id, dimid, len);
    }

    return_code = IOFW_IO_ERROR_NONE;

RETURN :
    if(NULL != name)
    {
	free(name);
	name = NULL;
    }
    return return_code;
}

int iofw_io_nc_def_var(int client_id)
{
    int ret = 0, i;
    int ncid, varid, ndims;
    int client_ncid, client_varid;
    iofw_id_nc_t *nc;
    iofw_id_dim_t **dims = NULL; 
    int *dimids = NULL, *client_dimids = NULL, *dims_len = NULL;
    char *name = NULL;
    nc_type xtype;

    int return_code;

    ret = iofw_msg_unpack_nc_def_var(&client_ncid, &name, &xtype, &ndims, 
	    &client_dimids, &client_varid);
    if( ret < 0 )
    {
        error("unpack_msg_def_var failed");
        return IOFW_IO_ERROR_MSG_UNPACK;
    }
    
    dims = malloc(ndims * sizeof(iofw_id_dim_t *));
    dims_len = malloc(ndims * sizeof(int));
    dimids = malloc(ndims * sizeof(int));
    
    if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_nc(client_ncid, &nc))
    {
	return_code = IOFW_IO_ERROR_INVALID_NC;
	goto RETURN;
    }
    for(i = 0; i < ndims; i ++)
    {
	if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_dim(
		    client_ncid, client_dimids[i], &dims[i]))
	{
	    return_code = IOFW_IO_ERROR_INVALID_DIM;
	    goto RETURN;
	}
	dimids[i] = dims[i]->dim_id;
	dims_len[i] = dims[i]->dim_len;
    }

    if(IOFW_ID_ERROR_GET_NULL ==
	    iofw_id_get_var(client_ncid, client_varid, &ncid, &varid))
    {
	ret = nc_def_var(ncid,name,xtype,ndims,dimids,&varid);
	if( ret != NC_NOERR )
	{
	    error("def var(%s) error(%s)",name,nc_strerror(ret));
	    return_code = IOFW_IO_ERROR_NC;
	    goto RETURN;
	}
	/* TODO size */
	iofw_id_map_var(client_ncid, client_varid, nc->ncid, varid,
		ndims, dims_len, sizeof(float));
    }
    return_code = IOFW_IO_ERROR_NONE;
    
RETURN :
    if(name != NULL)
    {
	free(name);
	name = NULL;
    }
    if(dims != NULL)
    {
	free(dims);
	dims = NULL;
    }
    if(client_dimids != NULL)
    {
	free(client_dimids);
	client_dimids = NULL;
    }
    if(dims_len != NULL)
    {
	free(dims_len);
	dims_len = NULL;
    }
    if(dimids != NULL)
    {
	free(dimids);
	dimids = NULL;
    }
    return return_code;
}

int iofw_io_nc_enddef(int client_id)
{
    int client_ncid, ret;
    iofw_id_nc_t *nc;

    ret = iofw_msg_unpack_nc_enddef(&client_ncid);
    if( ret < 0 )
    {
        error("unapck msg error");
        return IOFW_IO_ERROR_MSG_UNPACK;
    }

    if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_nc(client_ncid, &nc))
    {
	DEBUG_MSG(DEBUG_IO, "Invalid NC.");
	return IOFW_IO_ERROR_INVALID_NC;
    }

    if(DEFINE_MODE == nc->nc_status)
    {
	ret = nc_enddef(ncid);
	if(ret < 0)
	{
	    error("nc_enddef error(%s)",nc_strerror(ret));
	    return IOFW_IO_ERROR_NC;
	}
    }
    return IOFW_IO_ERROR_NONE;
}
int iofw_io_nc_put_vara_float(int client_id)
{
    int i,ret = 0,ncid, varid, dim;
    iofw_id_nc_t *nc;
    iofw_id_var_t *var;
    int client_ncid, client_varid;
    size_t *start, *count;
    size_t data_size;
    float *data;
    int data_len;

    int return_code;

//    ret = iofw_unpack_msg_extra_data_size(h_buf, &data_size);
    ret = iofw_msg_unpack_nc_put_vara_float(
	    &client_ncid, &client_varid, &dim, &start, &count,
	    &data_len, &data);	

    if( ret < 0 )
    {
	error("unpack_msg_put_vara_float failure");
	return IOFW_ID_ERROR_MSG_UNPACK;
    }

    if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_nc(client_ncid, &nc))
    {
	return_code = IOFW_IO_ERROR_INVALID_NC;
	debug(DEBUG_IO, "Invalid nc.");
	goto RETURN;
    }
    if(IOFW_ID_ERROR_GET_NULL == 
	    iofw_id_get_var(client_ncid, client_varid, &var))
    {
	return_code = IOFW_IO_ERROR_INVALID_VAR;
	debug(DEBUG_IO, "Invalid var.");
	goto RETURN;
    }

    if(iofw_id_put_var(client_ncid, client_varid, start, count, data) < 0)
    {
	debug(DEBUG_IO, "put var fail.");
	return_ocde = IOFW_IO_ERROR_PUT_VAR;
	goto RETURN;
    }

    var->recv_client_num ++;

    if(client_num == var->recv_client_num)
    {
	var->recv_client_num = 0;
	ret = nc_put_vara_float( ncid, varid, start, count, (float *)(data));
	if( ret != NC_NOERR )
	{
	    error("write nc(%d) var (%d) failure(%s)",ncid,varid,nc_strerror(ret));
	    return_code = -IOFW_IO_ERROR_NC;
	}
    }

    return_code = IOFW_IO_ERROR_NONE;	

RETURN :
    
    if(NULL != start)
    {
	free(start);
	start = NULL;
    }
    if(NULL != count)
    {
	free(count);
	count = NULL;
    }

    return return_code;

}

int iofw_io_nc_close(int client_id)
{
    int client_ncid, ncid, ret;
    iofw_id_nc_t *nc;

    ret = iofw_msg_unpack_nc_close(&client_ncid);

    if( ret < 0 )
    {
	error("unpack close error\n");
	return ret;
    }

    /*TODO handle memory free*/

    if(IOFW_ID_ERROR_GET_NULL == iofw_id_get_nc(client_ncid, &nc))
    {
	DEBUG_MSG(DEBUG_IO, "Invalid NC.");
	return IOFW_IO_ERROR_INVALID_NC;
    }
    ret = nc_close(nc->ncid);
    

    if( ret != NC_NOERR )
    {
        error("close nc %d file failure,%s\n",ncid,nc_strerror(ret));
        return IOFW_ID_ERROR_NC;
    }
    return IOFW_IO_ERROR_NONE;
}

/**
 * @brief iofw_nc_put_var1_float : 
 *
 * @param src: the rank of the client
 * @param my_rank: 
 * @param buf
 *
 * @return 
 */
int iofw_io_nc_put_var1_float(int client_id)
{
    return 0;	
}
