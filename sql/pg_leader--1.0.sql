CREATE FUNCTION get_node_id() RETURNS integer 
AS 'MODULE_PATHNAME', 'get_node_id' LANGUAGE C;

CREATE FUNCTION get_leader_id() RETURNS integer 
AS 'MODULE_PATHNAME', 'get_leader_id' LANGUAGE C;

create function pg_leader_cluster_config() returns text AS $$ 
BEGIN
    RETURN 'hello world';
END;
$$ LANGUAGE plpgsql;