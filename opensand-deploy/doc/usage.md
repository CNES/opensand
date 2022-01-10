# Configuring the OpenSAND Configuration Backend

The OpenSAND configuration manager will need to access the machines onto which you
want to run the OpenSAND process through SSH. But to keep things simple, the frontend
will only let you specify the IP address of the machine you are trying to reach
(you can optionally use the user@ip:port syntax, though) so every special configuration
that you need on your network like using private keys or the like should be specified
in the .ssh/config file of the opensand user on the machine you installed the
opensand-deploy packet.

# Using the OpenSAND Configuration Frontend

The configuration frontend is available on the port 80 in the machine you installed
it in. So just fire a browser to the IP of your install machine to access it.

## Managing projects

On the main interface, you can create a new project by clicking on the "Create" button
and specifying a name that doesn't already exist. You can also share projects with
other peoples through a tarball and import them in your own platform through the "Upload"
button.

Projects that you created can also be copied to a new project by clicking their "Copy"
button; or deleted by clicking their "Delete" button.

By clicking the "Download" button on a project you can retrieve a tarball that you can
share with others, or that you can use to deploy configuration files on you platform.

Lastly the "Open" button, or even the whole project card, let you access the project's
main configuration page.

## Adding a new entity to a platform

On the main project page, you have two tabs. The first one is to manage the machines of
your platform and specify informations to access them. You will be able to perform
actions on them through the list. Actions requiring a password or an SSH passphrase
will prompt for them and never store it.

On this "Platform" tab, you can add a new machine that you want to control. You must give
it a unique name and specify the kind of OpenSAND entity that you want to run on it. This
does not need to be a unique machine for each entity, but you must tell each entity apart
if you want to manage several OpenSAND processes on a single machine.

Once you’ve added a new entity, you can specify its IP address, where and how to push
configuration files onto it, and what kind of management action you want to perform.

## Configuring an entity

Once entities are added on the "Platform" tab, they also appear on the "Configuration" tab.
In there, you will be able to edit the configuration files for each of your entities.

You also have the ability to save a configuration file as a template for others entities
to not start anew each time. For instance, you can provide specific log levels in an
infrastructure configuration file, save it as a template and others entities will be
able to reuse the same level configured when you select the template that you just created.

## Deployment and Emulation

Once an entity is fully configured, you can start the OpenSAND binary on the machine by
choosing the SSH "Run Method" in the "Platform" tab. Don't forget to specify in which
folder the configuration files must be pushed into. If the OpenSAND binary starts
successfully, a progression circle will replace the launch button: as long as the process
is running on the machine, the circle will spin. If the process stops, the launch button
is displayed back.

You can manually stop the OpenSAND binary by selecting the STOP "Run Method". Success
can be monitored by switching back to the SSH method and checking wether the launch
button or the progress circle is displayed.

## Testing

You can test emulated links by using the PING "Run Method". Upon starting a ping, you
must provide a destination address to ping. The interface will assist you by displaying
a list of your emulation addresses, so you can choose one to ping from the entity you're
starting your ping with; or use an other address of your liking.

The result of the ping will be displayed shortly after in a new dialog window.
