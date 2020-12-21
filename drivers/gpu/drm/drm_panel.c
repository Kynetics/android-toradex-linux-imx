/*
 * Copyright (C) 2013, NVIDIA Corporation.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <linux/err.h>
#include <linux/module.h>

#include <drm/drm_crtc.h>
#include <drm/drm_panel.h>

static DEFINE_MUTEX(panel_lock);
static LIST_HEAD(panel_list);

/**
 * DOC: drm panel
 *
 * The DRM panel helpers allow drivers to register panel objects with a
 * central registry and provide functions to retrieve those panels in display
 * drivers.
 *
 * For easy integration into drivers using the &drm_bridge infrastructure please
 * take look at drm_panel_bridge_add() and devm_drm_panel_bridge_add().
 */

/**
 * drm_panel_init - initialize a panel
 * @panel: DRM panel
 *
 * Sets up internal fields of the panel so that it can subsequently be added
 * to the registry.
 */
void drm_panel_init(struct drm_panel *panel)
{
	INIT_LIST_HEAD(&panel->list);
	BLOCKING_INIT_NOTIFIER_HEAD(&panel->nh);
}
EXPORT_SYMBOL(drm_panel_init);

/**
 * drm_panel_add - add a panel to the global registry
 * @panel: panel to add
 *
 * Add a panel to the global registry so that it can be looked up by display
 * drivers.
 *
 * Return: 0 on success or a negative error code on failure.
 */
int drm_panel_add(struct drm_panel *panel)
{
	mutex_lock(&panel_lock);
	list_add_tail(&panel->list, &panel_list);
	mutex_unlock(&panel_lock);

	return 0;
}
EXPORT_SYMBOL(drm_panel_add);

/**
 * drm_panel_remove - remove a panel from the global registry
 * @panel: DRM panel
 *
 * Removes a panel from the global registry.
 */
void drm_panel_remove(struct drm_panel *panel)
{
	mutex_lock(&panel_lock);
	list_del_init(&panel->list);
	mutex_unlock(&panel_lock);
}
EXPORT_SYMBOL(drm_panel_remove);

/**
 * drm_panel_attach - attach a panel to a connector
 * @panel: DRM panel
 * @connector: DRM connector
 *
 * After obtaining a pointer to a DRM panel a display driver calls this
 * function to attach a panel to a connector.
 *
 * An error is returned if the panel is already attached to another connector.
 *
 * When unloading, the driver should detach from the panel by calling
 * drm_panel_detach().
 *
 * Return: 0 on success or a negative error code on failure.
 */
int drm_panel_attach(struct drm_panel *panel, struct drm_connector *connector)
{
	if (panel->connector)
		return -EBUSY;

	panel->connector = connector;
	panel->drm = connector->dev;

	return 0;
}
EXPORT_SYMBOL(drm_panel_attach);

/**
 * drm_panel_detach - detach a panel from a connector
 * @panel: DRM panel
 *
 * Detaches a panel from the connector it is attached to. If a panel is not
 * attached to any connector this is effectively a no-op.
 *
 * This function should not be called by the panel device itself. It
 * is only for the drm device that called drm_panel_attach().
 */
void drm_panel_detach(struct drm_panel *panel)
{
	panel->connector = NULL;
	panel->drm = NULL;
}
EXPORT_SYMBOL(drm_panel_detach);

/**
 * drm_panel_prepare - power on a panel
 * @panel: DRM panel
 *
 * Calling this function will enable power and deassert any reset signals to
 * the panel. After this has completed it is possible to communicate with any
 * integrated circuitry via a command bus.
 *
 * Return: 0 on success or a negative error code on failure.
 */
int drm_panel_prepare(struct drm_panel *panel)
{
	if (panel && panel->funcs && panel->funcs->prepare)
		return panel->funcs->prepare(panel);

	return panel ? -ENOSYS : -EINVAL;
}
EXPORT_SYMBOL(drm_panel_prepare);

/**
 * drm_panel_unprepare - power off a panel
 * @panel: DRM panel
 *
 * Calling this function will completely power off a panel (assert the panel's
 * reset, turn off power supplies, ...). After this function has completed, it
 * is usually no longer possible to communicate with the panel until another
 * call to drm_panel_prepare().
 *
 * Return: 0 on success or a negative error code on failure.
 */
int drm_panel_unprepare(struct drm_panel *panel)
{
	if (panel && panel->funcs && panel->funcs->unprepare)
		return panel->funcs->unprepare(panel);

	return panel ? -ENOSYS : -EINVAL;
}
EXPORT_SYMBOL(drm_panel_unprepare);

/**
 * drm_panel_enable - enable a panel
 * @panel: DRM panel
 *
 * Calling this function will cause the panel display drivers to be turned on
 * and the backlight to be enabled. Content will be visible on screen after
 * this call completes.
 *
 * Return: 0 on success or a negative error code on failure.
 */
int drm_panel_enable(struct drm_panel *panel)
{
	if (panel && panel->funcs && panel->funcs->enable)
		return panel->funcs->enable(panel);

	return panel ? -ENOSYS : -EINVAL;
}
EXPORT_SYMBOL(drm_panel_enable);

/**
 * drm_panel_disable - disable a panel
 * @panel: DRM panel
 *
 * This will typically turn off the panel's backlight or disable the display
 * drivers. For smart panels it should still be possible to communicate with
 * the integrated circuitry via any command bus after this call.
 *
 * Return: 0 on success or a negative error code on failure.
 */
int drm_panel_disable(struct drm_panel *panel)
{
	if (panel && panel->funcs && panel->funcs->disable)
		return panel->funcs->disable(panel);

	return panel ? -ENOSYS : -EINVAL;
}
EXPORT_SYMBOL(drm_panel_disable);

/**
 * drm_panel_get_modes - probe the available display modes of a panel
 * @panel: DRM panel
 *
 * The modes probed from the panel are automatically added to the connector
 * that the panel is attached to.
 *
 * Return: The number of modes available from the panel on success or a
 * negative error code on failure.
 */
int drm_panel_get_modes(struct drm_panel *panel)
{
	if (panel && panel->funcs && panel->funcs->get_modes)
		return panel->funcs->get_modes(panel);

	return panel ? -ENOSYS : -EINVAL;
}
EXPORT_SYMBOL(drm_panel_get_modes);

#ifdef CONFIG_OF
/**
 * of_drm_find_panel - look up a panel using a device tree node
 * @np: device tree node of the panel
 *
 * Searches the set of registered panels for one that matches the given device
 * tree node. If a matching panel is found, return a pointer to it.
 *
 * Return: A pointer to the panel registered for the specified device tree
 * node or an ERR_PTR() if no panel matching the device tree node can be found.
 *
 * Possible error codes returned by this function:
 *
 * - EPROBE_DEFER: the panel device has not been probed yet, and the caller
 *   should retry later
 * - ENODEV: the device is not available (status != "okay" or "ok")
 */
struct drm_panel *of_drm_find_panel(const struct device_node *np)
{
	struct drm_panel *panel;

	if (!of_device_is_available(np))
		return ERR_PTR(-ENODEV);

	mutex_lock(&panel_lock);

	list_for_each_entry(panel, &panel_list, list) {
		if (panel->dev->of_node == np) {
			mutex_unlock(&panel_lock);
			return panel;
		}
	}

	mutex_unlock(&panel_lock);
	return ERR_PTR(-EPROBE_DEFER);
}
EXPORT_SYMBOL(of_drm_find_panel);
#endif

int drm_panel_notifier_register(struct drm_panel *panel,
	struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&panel->nh, nb);
}
EXPORT_SYMBOL(drm_panel_notifier_register);

int drm_panel_notifier_unregister(struct drm_panel *panel,
	struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&panel->nh, nb);
}
EXPORT_SYMBOL(drm_panel_notifier_unregister);

int drm_panel_notifier_call_chain(struct drm_panel *panel,
	unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&panel->nh, val, v);
}
EXPORT_SYMBOL(drm_panel_notifier_call_chain);

MODULE_AUTHOR("Thierry Reding <treding@nvidia.com>");
MODULE_DESCRIPTION("DRM panel infrastructure");
MODULE_LICENSE("GPL and additional rights");
