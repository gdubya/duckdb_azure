#define DUCKDB_EXTENSION_MAIN

#include "azure_extension.hpp"
#include "azure_filesystem.hpp"
#include "azure_secret.hpp"

namespace duckdb {

static void LoadInternal(DatabaseInstance &instance) {
	// Load filesystem
	auto &fs = instance.GetFileSystem();
	fs.RegisterSubSystem(make_uniq<AzureStorageFileSystem>());

	// Load Secret functions
	CreateAzureSecretFunctions::Register(instance);

	// Load extension config
	auto &config = DBConfig::GetConfig(instance);
	config.AddExtensionOption("azure_storage_connection_string",
	                          "Azure connection string, used for authenticating and configuring azure requests",
	                          LogicalType::VARCHAR);
	config.AddExtensionOption(
	    "azure_account_name",
	    "Azure account name, when set, the extension will attempt to automatically detect credentials",
	    LogicalType::VARCHAR);
	config.AddExtensionOption("azure_credential_chain",
	                          "Ordered list of Azure credential providers, in string format separated by ';'. E.g. "
	                          "'cli;managed_identity;env'",
	                          LogicalType::VARCHAR, "none");
	config.AddExtensionOption("azure_endpoint",
	                          "Override the azure endpoint for when the Azure credential providers are used.",
	                          LogicalType::VARCHAR, "blob.core.windows.net");
	config.AddExtensionOption("azure_http_stats",
	                          "Include http info from the Azure Storage in the explain analyze statement. "
	                          "Notice that the result may be incorrect for more than one active DuckDB connection "
	                          "and the calculation of total received and sent bytes is not yet implemented.",
	                          LogicalType::BOOLEAN, false);
	config.AddExtensionOption("azure_context_caching",
	                          "Enable/disable the caching of some context when performing queries. "
	                          "This cache is by default enable, and will for a given connection keep a local context "
	                          "when performing a query. "
	                          "If you suspect that the caching is causing some side effect you can try to disable it "
	                          "by setting this option to false.",
	                          LogicalType::BOOLEAN, true);

	// SPN configuration entries
	config.AddExtensionOption("azure_tenant_id", "The Microsoft Entra tenant (directory) ID.", LogicalType::VARCHAR);
	config.AddExtensionOption("azure_spn_client_id",
	                          "The client (application) ID of an App Registration in the tenant.",
	                          LogicalType::VARCHAR);
	config.AddExtensionOption("azure_spn_client_secret", "A client secret that was generated for the App Registration.",
	                          LogicalType::VARCHAR);
	config.AddExtensionOption(
	    "azure_spn_client_certificate_path",
	    "A path to certificate and private key pair in PEM or PFX format, which can authenticate the App Registration.",
	    LogicalType::VARCHAR);

	AzureReadOptions default_read_options;
	config.AddExtensionOption("azure_read_transfer_concurrency",
	                          "Maximum number of threads the Azure client can use for a single parallel read. "
	                          "If azure_read_transfer_chunk_size is less than azure_read_buffer_size then setting "
	                          "this > 1 will allow the Azure client to do concurrent requests to fill the buffer.",
	                          LogicalType::INTEGER, Value::INTEGER(default_read_options.transfer_concurrency));

	config.AddExtensionOption("azure_read_transfer_chunk_size",
	                          "Maximum size in bytes that the Azure client will read in a single request. "
	                          "It is recommended that this is a factor of azure_read_buffer_size.",
	                          LogicalType::BIGINT, Value::BIGINT(default_read_options.transfer_chunk_size));

	config.AddExtensionOption("azure_read_buffer_size",
	                          "Size of the read buffer.  It is recommended that this is evenly divisible by "
	                          "azure_read_transfer_chunk_size.",
	                          LogicalType::UBIGINT, Value::UBIGINT(default_read_options.buffer_size));

	auto *http_proxy = std::getenv("HTTP_PROXY");
	Value default_http_value = http_proxy ? Value(http_proxy) : Value(nullptr);
	config.AddExtensionOption("azure_http_proxy",
	                          "Proxy to use when login & performing request to azure. "
	                          "By default it will use the HTTP_PROXY environment variable if set.",
	                          LogicalType::VARCHAR, default_http_value);
	config.AddExtensionOption("azure_proxy_user_name", "Http proxy user name if needed.", LogicalType::VARCHAR,
	                          Value(nullptr));
	config.AddExtensionOption("azure_proxy_password", "Http proxy password if needed.", LogicalType::VARCHAR,
	                          Value(nullptr));
}

void AzureExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string AzureExtension::Name() {
	return "azure";
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void azure_init(duckdb::DatabaseInstance &db) {
	LoadInternal(db);
}

DUCKDB_EXTENSION_API const char *azure_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
