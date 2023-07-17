CREATE FUNCTION pg_leader_node_id() RETURNS integer 
AS 'MODULE_PATHNAME',

create function pg_leader_cluster_config() returns text AS $$ 
BEGIN
    RETURN 'hello world';
END;
$$ LANGUAGE plpgsql;