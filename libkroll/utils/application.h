/**
 * Appcelerator Kroll - licensed under the Apache Public License 2
 * see LICENSE in the root folder for details on the license.
 * Copyright (c) 2009 Appcelerator, Inc. All Rights Reserved.
 */
#ifndef _KR_UTILS_APPLICATION_H_
#define _KR_UTILS_APPLICATION_H_

namespace UTILS_NS
{
	using std::string;
	using std::vector;
	using std::pair;
	using std::map;

	/**
	 * Represents a concrete Kroll application -- found on disk
	 */
	class KROLL_API Application
	{
	private:
		Application(const std::string &path, const std::string &manifestPath);
		void ParseManifest(const map<string, string>& manifest);

		const string path;
		const string manifestPath;

		string name;
		string version;
		string id;
		string guid;
		string url;
		string publisher;
		string image;
		string logLevel;

		string stream;
		string queryString;

	public:

		vector<string> arguments;
		vector<SharedDependency> dependencies;
		vector<SharedComponent> modules;
		vector<SharedComponent> sdks;
		SharedComponent runtime;

		static bool doesManifestFileExistsAtDirectory(const std::string & dir);

		static SharedApplication NewApplication(const std::string &appPath);
		static SharedApplication NewApplication(const std::string &manifestPath, const std::string &applicationPath);
		// special in-memory constructor, no paths
		static SharedApplication NewApplication(const map<string, string>& manifest);
		~Application();

		string getPath() const { return this->path; }
		string getManifestPath() const { return this->manifestPath; }
		string getName() const { return this->name; }
		string getVersion() const { return this->version; }
		string getId() const { return this->id; }
		string getGUID() const { return this->guid; }
		string getURL() const { return this->url; }
		string getPublisher() const { return this->publisher; }
		string getImage() const { return this->image; }
		string getLogLevel() const { return this->logLevel; }

		/**
		 * Get the path to this application's executablej
		 */
		string GetExecutablePath() const;

		/**
		 * Get an active component path given a name.
		 * @arg name a component name either the name of a module (e.g. 'tiui') or 'runtime'
		 * @returns the path to the component with the given name or an empty string if not found
		 */
		string GetComponentPath(const string &name) const;

		/**
		 * Get the path to this application's user data directory.
		 */
		string GetDataPath() const;

		/**
		 * Get the path to this application's resources directory.
		 */
		string GetResourcesPath() const;

		/**
		 * Get the text of a license file for this application or an empty string if
		 * no license is found.
		 */
		std::string GetLicenseText() const;

		/**
		 * Whether or not this application has a .installed file in it's path
		 */
		bool IsInstalled() const;

		/**
		 * Try to resolve all application dependencies with installed or bundled components.
		 * @returns a list of unresolved dependencies
		 */
		vector<SharedDependency> ResolveDependencies();

		/**
		 * Get the URL for a particular dependency or the path to a bundled .zip file
		 * if it is found.
		 */
		string GetURLForDependency(SharedDependency d);

		/**
		 * Construct an appropriate URL to get *this* version of the application. For instance,
		 * to get an update for an application, construct it using the update manifest and
		 * then call this method on the resulting Application.
		 */
		std::string GetUpdateURL();

		/**
		 * Get the stream URL for this application
		 */
		std::string& GetStreamURL(const char* scheme="http");

		/**
		 * Generate a list of all components available for this application
		 * including bundled components and any components or all the components
		 * in the bundle override directory.
		 */
		void GetAvailableComponents(
			vector<SharedComponent>& components,
			bool onlyBundled = false);

		/**
		 * Inform the application that it is using a module with the given
		 * name and version. If this is a new module, it will be registered in
		 * the application's module list.
		 */
		void UsingModule(
			const std::string &name,
			const std::string &version,
			const std::string &path);

		/**
		 * A mutator for this application's list of command-line arguments.
		 */
		void SetArguments(int argc, const char* argv[]);

		/**
		 * A mutator for this application's list of command-line arguments.
		 */
		void SetArguments(const vector<string>& arguments);

		/**
		 * An accessor for this application's list of command-line arguments.
		 */
		vector<string>& GetArguments();

		/**
		 * Whether or not the given argument was specified on the command-line. If "--arg"
		 * was specified, a needle equalling "--arg" or "arg" will return true.
		 */
		bool HasArgument(const string &needle) const;

		/**
		 * Get the value of an argument that was specified like arg=value or arg="value"
		 * @returns argument value or an empty string if not found
		 */
		string GetArgumentValue(const string &needle) const;

		/**
		 * Get all resolved components for this application including
		 * runtimes, sdks and modules.
		 */
		void GetResolvedComponents(vector<SharedComponent> &resolved);
	};
}
#endif
