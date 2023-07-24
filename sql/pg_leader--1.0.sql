CREATE FUNCTION get_node_info() RETURNS cstring
AS 'MODULE_PATHNAME', 'get_node_info' LANGUAGE C;

create function pg_leader_cluster_config() returns text AS $$ 
BEGIN
    RETURN 'hello world';
END;
$$ LANGUAGE plpgsql;