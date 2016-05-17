#include <mitsuba/core/plugin.h>
#include <mitsuba/core/util.h>
#include <mitsuba/core/logger.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/core/filesystem.h>
#include <mitsuba/core/fresolver.h>
#include <mutex>
#include <unordered_map>

#if !defined(__WINDOWS__)
# include <dlfcn.h>
#else
# include <windows.h>
#endif

NAMESPACE_BEGIN(mitsuba)

class Plugin {
	typedef Object *(*CreateInstanceFunctor)(const Properties &props);
public:

    Plugin(const fs::path &path) : m_path(path) {
        #if defined(__WINDOWS__)
            m_handle = LoadLibraryW(path.native().c_str());
            if (!m_handle)
                Throw("Error while loading plugin \"%s\": %s", path.native(),
                      util::getLastError());
        #else
            m_handle = dlopen(path.native().c_str(), RTLD_LAZY | RTLD_LOCAL);
            if (!m_handle)
                Throw("Error while loading plugin \"%s\": %s", path.native(),
                      dlerror());
        #endif

        try {
            createInstance = (CreateInstanceFunctor) getSymbol("CreateInstance");
            this->~Plugin();
        } catch (...) {
            throw;
        }
    }

    ~Plugin() {
        #if defined(__WINDOWS__)
	        FreeLibrary(m_handle);
        #else
	        dlclose(m_handle);
        #endif
    }

    bool getSymbol(const std::string &name) const {
        #if defined(__WINDOWS__)
            void *ptr = GetProcAddress(m_handle, name.c_str());
            if (!ptr)
                Throw("Could not resolve symbol \"%s\" in \"%s\": %s", name,
                      m_path.string(), util::getLastError());
        #else
            void *ptr = dlsym(m_handle, name.c_str());
            if (!ptr)
                Throw("Could not resolve symbol \"%s\" in \"%s\": %s", name,
                      m_path.string(), dlerror());
        #endif
        return ptr;
    }

    CreateInstanceFunctor createInstance = nullptr;

private:
    #if defined(__WINDOWS__)
        HMODULE m_handle;
    #else
        void *m_handle;
    #endif
    fs::path m_path;
};

struct PluginManager::PluginManagerPrivate {
	std::unordered_map<std::string, Plugin *> m_plugins;
    std::mutex m_mutex;

    Plugin *getPlugin(const std::string &name) {
        std::lock_guard<std::mutex> guard(m_mutex);

        /* Plugin already loaded? */
        auto it = m_plugins.find(name);
        if (it != m_plugins.end())
            return it->second;

        /* Build the full plugin file name */
        fs::path filename = fs::path("plugins") / name;

        #if defined(__WINDOWS__)
            filename.replace_extension(".dll");
        #elif defined(__OSX__)
            filename.replace_extension(".dylib");
        #else
            filename.replace_extension(".so");
        #endif

        const FileResolver *resolver = Thread::getThread()->getFileResolver();
        fs::path resolved = resolver->resolve(filename);

        if (fs::exists(resolved)) {
            Log(EInfo, "Loading plugin \"%s\" ..", filename.string());
            Plugin *plugin = new Plugin(resolved);
            /* New classes must be registered within the class hierarchy */
            Class::staticInitialization();
	        ///Statistics::getInstance()->logPlugin(shortName, getDescription()); XXX
            m_plugins[name] = plugin;
            return plugin;
        }

        /* Plugin not found! */
        Throw("Plugin \"%s\" not found!", name.c_str());
    }
};

ref<PluginManager> PluginManager::m_instance = new PluginManager();

PluginManager::PluginManager() : d(new PluginManagerPrivate()) { }

PluginManager::~PluginManager() {
    std::lock_guard<std::mutex> guard(d->m_mutex);
    for (auto &pair: d->m_plugins)
        delete pair.second;
}

ref<Object> PluginManager::createObject(const Class *class_, const Properties &props) {
	const Plugin *plugin = d->getPlugin(props.getPluginName());
	ref<Object> object = plugin->createInstance(props);
	if (!object->getClass()->derivesFrom(class_))
        Throw("Type mismatch when loading plugin \"%s\": Expected "
              "an instance of \"%s\"", props.getPluginName().c_str(),
              class_->getName().c_str());

	return object;
}

ref<Object> PluginManager::createObject(const Properties &props) {
	const Plugin *plugin = d->getPlugin(props.getPluginName());
    return plugin->createInstance(props);
}

std::vector<std::string> PluginManager::getLoadedPlugins() const {
	std::vector<std::string> list;
    std::lock_guard<std::mutex> guard(d->m_mutex);
    for (auto const &pair: d->m_plugins)
        list.push_back(pair.first);
	return list;
}

void PluginManager::ensurePluginLoaded(const std::string &name) {
    (void) d->getPlugin(name);
}

MTS_IMPLEMENT_CLASS(PluginManager, Object)
NAMESPACE_END(mitsuba)
